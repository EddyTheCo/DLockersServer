#include"bserver.hpp"
#include <QCryptographicHash>
#include <QDebug>
#include<QJsonDocument>
#include<QTimer>
#include <QRandomGenerator>
#include"Day_model.hpp"
#include<account.hpp>
#include<nodeConnection.hpp>
#if defined(RPI_SERVER)
#include"lgpio.h"
#include <QGuiApplication>
#if QT_CONFIG(permissions)
#include <QPermission>
#endif
#endif

using namespace qiota::qblocks;

using namespace qiota;
using namespace qcrypto;

void Book_Server::init()
{
    auto info=Node_Conection::instance()->rest()->get_api_core_v2_info();
    QObject::connect(info,&Node_info::finished,reciever,[=]( ){

        auto resp2=Node_Conection::instance()->mqtt()->
                     get_outputs_unlock_condition_address("address/"+Account::instance()->addr_bech32({0,0,0},info->bech32Hrp));
        connect(resp2,&ResponseMqtt::returned,reciever,[=](QJsonValue data)
                {
                    if(!started)
                    {
                        this->handle_init_funds();
                    }
                    checkFunds({Node_output(data)});

                });
        this->handle_init_funds();

        auto resp=Node_Conection::instance()->mqtt()->
                    get_outputs_unlock_condition_address("address/"+Account::instance()->addr_bech32({0,0,1},info->bech32Hrp));
        QObject::connect(resp,&ResponseMqtt::returned,reciever,[=](QJsonValue data){

            if(state_==Ready)
            {
                handle_new_book(Node_output(data));
            }
            else
            {
                queue.push(Node_output(data));
            }
        });
        info->deleteLater();
    });
}
void Book_Server::check_state_output(const std::vector<Node_output> node_output_s)
{
    if(node_output_s.size())
    {
        const auto basic_output_=node_output_s.front().output();
        auto metfeau=basic_output_->get_feature_(Feature::Metadata_typ);
        if(metfeau)
        {
            auto metadata_feature=std::static_pointer_cast<const Metadata_Feature>(metfeau);
            auto metadata=metadata_feature->data();
            deserialize_state(metadata);
        }

    }
}
void Book_Server::get_restart_state(void)
{
    auto info=Node_Conection::instance()->rest()->get_api_core_v2_info();
    connect(info,&Node_info::finished,reciever,[=]( ){
        setServerId(Account::instance()->addr_bech32({0,0,0},info->bech32Hrp));
        auto node_outputs_=new Node_outputs();
        connect(node_outputs_,&Node_outputs::finished,reciever,[=]( ){
            check_state_output(node_outputs_->outs_);
            init();
            node_outputs_->deleteLater();
        });
        Node_Conection::instance()->rest()->get_outputs<Output::Basic_typ>(node_outputs_,"address="+
                                                                                              Account::instance()->addr_bech32({0,0,0},info->bech32Hrp)+
                                                                                              "&hasStorageDepositReturn=false&hasTimelock=false&hasExpiration=false&sender="+
                                                                                              Account::instance()->addr_bech32({0,0,0},info->bech32Hrp)+"&tag="+fl_array<quint8>("state").toHexString());
        auto node_outputs=new Node_outputs();
        connect(node_outputs,&Node_outputs::finished,reciever,[=]( ){
            checkFunds(node_outputs->outs_);
            node_outputs->deleteLater();
        });
        Node_Conection::instance()->rest()->get_outputs<Output::Basic_typ>(node_outputs,"address="+
                                                                                             Account::instance()->addr_bech32({0,0,0},info->bech32Hrp));

        info->deleteLater();
    });

}
void Book_Server::restart(void)
{
    if(Node_Conection::instance()->state()==Node_Conection::Connected)
    {
        if(reciever)reciever->deleteLater();
        reciever=new QObject(this);
        setFunds(0);
        setminFunds(1000000);
        total_funds.clear();
        payments_=QJsonArray();
        connect(this,&Book_Server::stateChanged,reciever,[=](){
            while (state_==Ready&&!queue.empty()) {
                handle_new_book(queue.front());
                queue.pop();
            }
        });
        get_restart_state();
    }

}
void Book_Server::checkFunds(std::vector<qiota::Node_output>  outs)
{
    quint64 total=0;
    for(const auto& v:outs)
    {
        std::vector<Node_output> var{v};
        auto bundle= Account::instance()->get_addr({0,0,0});
        bundle.consume_outputs(var);

        if(bundle.amount)
        {
            total+=bundle.amount;
            total_funds.insert(v.metadata().outputid_.toHexString(),bundle.amount);
            auto resp=Node_Conection::instance()->mqtt()->get_outputs_outputId(v.metadata().outputid_.toHexString());
            connect(resp,&ResponseMqtt::returned,reciever,[=](QJsonValue data){
                const auto node_output=Node_output(data);
                if(node_output.metadata().is_spent_)
                {
                    auto it=total_funds.constFind(node_output.metadata().outputid_.toHexString());
                    if(it!=total_funds.cend())
                    {
                        setFunds(funds_-it.value());
                        total_funds.erase(it);
                    }
                    resp->deleteLater();
                }

            });
        }
        if(bundle.to_expire.size())
        {
            const auto unixtime=bundle.to_expire.front();
            const auto triger=(unixtime-QDateTime::currentDateTime().toSecsSinceEpoch())*1000;
            QTimer::singleShot(triger,reciever,[=](){
                auto it=total_funds.constFind(v.metadata().outputid_.toHexString());
                if(it!=total_funds.cend())
                {
                    setFunds(funds_-it.value());
                    total_funds.erase(it);
                }
            });
        }
        if(bundle.to_unlock.size())
        {
            const auto unixtime=bundle.to_unlock.front();
            const auto triger=(unixtime-QDateTime::currentDateTime().toSecsSinceEpoch())*1000;
            QTimer::singleShot(triger+5000,reciever,[=](){
                auto resp=Node_Conection::instance()->mqtt()->get_outputs_outputId(v.metadata().outputid_.toHexString());
                connect(resp,&ResponseMqtt::returned,reciever,[=](QJsonValue data){
                    const auto node_output=Node_output(data);
                    checkFunds({node_output});
                    resp->deleteLater();
                });
            });
        }

    }
    setFunds(funds_+total);
}
void Book_Server::setFunds(quint64 funds_m){

    if(funds_!=funds_m||funds_m==0)
    {
        funds_=funds_m;
        auto info=Node_Conection::instance()->rest()->get_api_core_v2_info();
        QObject::connect(info,&Node_info::finished,reciever,[=]( ){
            funds_json=info->amount_json(funds_);
            emit fundsChanged();
            info->deleteLater();
        });
    }
}
void Book_Server::setminFunds(quint64 funds_m){
    auto info=Node_Conection::instance()->rest()->get_api_core_v2_info();
    QObject::connect(info,&Node_info::finished,reciever,[=]( ){
        minfunds_json=info->amount_json(funds_m);
        emit minfundsChanged();
        info->deleteLater();
    });

}
Book_Server::Book_Server(QObject *parent):QObject(parent),price_per_hour_(10000),state_(Ready),reciever(nullptr),started(false),
    open(false)
#if defined(RPI_SERVER)
    ,m_rpi_server(true),m_GeoCoord(41.902229,12.458100)
#else
    ,m_rpi_server(false)
#endif

{
#if defined(RPI_SERVER)
    checkLPermission();
#endif
}
#if defined(RPI_SERVER)
void Book_Server::initGPS(void)
{
    PosSource = QGeoPositionInfoSource::createSource("nmea", {std::make_pair("nmea.source",SERIAL_PORT_NAME)}, this);
    if(!PosSource)PosSource=QGeoPositionInfoSource::createDefaultSource(this);
    if (PosSource) {
        qDebug()<<"PosSource:"<<PosSource->sourceName();
        connect(PosSource,&QGeoPositionInfoSource::positionUpdated,
                this, [=](const QGeoPositionInfo &update){
                    m_GeoCoord=update.coordinate();
                    emit geoCoordChanged();
                });
        connect(PosSource,&QGeoPositionInfoSource::errorOccurred,
                this, [=](QGeoPositionInfoSource::Error _t1){
                    qDebug()<<"Position Error:"<<_t1;
                });
        PosSource->setUpdateInterval(30000);
        PosSource->requestUpdate();
        PosSource->startUpdates();
    }
}
void Book_Server::checkLPermission(void)
{

#if QT_CONFIG(permissions)

    QLocationPermission lPermission;
    lPermission.setAccuracy(QLocationPermission::Precise);
    switch (qApp->checkPermission(lPermission)) {
    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission(lPermission, this,
                                &Book_Server::checkLPermission);
        return;
    case Qt::PermissionStatus::Denied:
        return;
    case Qt::PermissionStatus::Granted:
        initGPS();
        return;
    }
#else
    initGPS();
#endif



}
#endif
void Book_Server::clean_state(void)
{
    for(const auto& v: books_)
    {
        if(!v.check_validity(QDateTime::currentDateTime()))
        {
            books_.erase(books_.find(v));
        }
    }
}

std::shared_ptr<qblocks::Output> Book_Server::get_publish_output(const quint64 &amount)const
{

    const fl_array<quint8> tag("state");
    const auto state=serialize_state();
    const auto eddAddr=Account::instance()->get_addr({0,0,0}).get_address();
    auto sendFea=Feature::Sender(eddAddr);
    auto tagFea=Feature::Tag(tag);

    auto metFea=Feature::Metadata(state);

    auto addUnlcon=Unlock_Condition::Address(eddAddr);
    return Output::Basic(amount,{addUnlcon},{},{sendFea,metFea,tagFea});
}
#if defined(RPI_SERVER)
void Book_Server::open_rpi_box(void)
{
    auto chip = lgGpiochipOpen(0);
    lgGpioSetUser(chip, "esterv");
    auto err = lgGpioClaimOutput(chip, 0, GPIO_NUMBER, 0);
    if (err) qDebug()<<"Set out err"<<err;
    lgGpioWrite(chip, GPIO_NUMBER, 1);
    qDebug()<<"opening gpio:"<<GPIO_NUMBER;
    QTimer::singleShot(2000,this,[=](){
        lgGpioWrite(chip, GPIO_NUMBER, 0);
        lgGpiochipClose(chip);
    });

}
#endif
void Book_Server::check_nft_to_open(Node_output node_out)
{

    if(node_out.output()->type()==Output::NFT_typ)
    {
        const auto output=node_out.output();
        const auto issuerfeature=output->get_immutable_feature_(Feature::Issuer_typ);
        if(issuerfeature)
        {
            const auto issuer=std::static_pointer_cast<const Issuer_Feature>(issuerfeature)->issuer()->addr();
            if(issuer==Account::instance()->get_addr({0,0,0}).get_address()->addr())
            {

                const auto metfeature=output->get_immutable_feature_(Feature::Metadata_typ);
                if(metfeature)
                {
                    const auto metadata=std::static_pointer_cast<const Metadata_Feature>(metfeature)->data();

                    auto books=Booking::get_new_bookings_from_metadata(metadata);

                    auto vec=Booking::from_Array(books);
                    auto now=QDateTime::currentDateTime().toSecsSinceEpoch();


                    for(auto book: vec )
                    {
                        if(book["finish"].toInteger()>=now&&book["start"].toInteger()<=now)
                        {
#if defined(RPI_SERVER)
                            open_rpi_box();
#endif
                            open=true;
                            emit openChanged();
                            break;

                        }
                    }

                }

            }
        }
    }
}

void Book_Server::handle_new_book(Node_output node_out)
{
    set_state(Publishing);
    QTimer::singleShot(15000,this,[=](){set_state(Ready);});
    if(node_out.output()->type()==Output::Basic_typ)
    {
        auto info=Node_Conection::instance()->rest()->get_api_core_v2_info();
        connect(info,&Node_info::finished,this,[=]( ){

            auto publish_node_outputs_= new Node_outputs();

            connect(publish_node_outputs_,&Node_outputs::finished,this,[=]( ){
                const auto basic_output_=node_out.output();
                const auto metfeature=basic_output_->get_feature_(Feature::Metadata_typ);
                if(metfeature)
                {
                    auto metadata_feature=std::static_pointer_cast<const Metadata_Feature>(metfeature);
                    auto metadata=metadata_feature->data();

                    auto new_books=Booking::get_new_bookings_from_metadata(metadata);

                    const auto now=QDateTime::currentDateTime().toSecsSinceEpoch();
                    const auto sendfeature=basic_output_->get_feature_(Feature::Sender_typ);
                    if(sendfeature)
                    {
                        auto sender=std::static_pointer_cast<const Sender_Feature>(sendfeature)->sender();


                        if(!new_books.isEmpty())
                        {

                            auto output=std::vector<Node_output>{node_out};
                            auto payment_bundle=Account::instance()->get_addr({0,0,1});
                            payment_bundle.consume_outputs({output});

                            QJsonArray varBooks;
                            auto vec=Booking::from_Array(new_books);
                            quint64 price=0;
                            quint64 maxB=0;

                            for(auto i=0;i<vec.size();i++)
                            {
                                if(vec[i]["finish"].toInteger()>vec[i]["start"].toInteger()&&
                                    vec[i]["start"].toInteger()>0&&
                                    vec[i]["finish"].toInteger()<QDateTime::currentDateTime().addDays(7).toSecsSinceEpoch())
                                {
                                    auto pair=books_.insert(vec.at(i));
                                    if(pair.second)
                                    {
                                        price+=vec.at(i).calculate_price(price_per_hour_);

                                        if(payment_bundle.amount<price)
                                        {
                                            books_.erase(vec.at(i));
                                            price-=vec.at(i).calculate_price(price_per_hour_);
                                            break;
                                        }
                                        varBooks.push_back(new_books.at(2*i));
                                        varBooks.push_back(new_books.at(2*i+1));
                                        if(new_books.at(2*i+1).toInteger()>maxB)maxB=new_books.at(2*i+1).toInteger();
                                    }
                                }
                            }


                            if(!varBooks.isEmpty())
                            {
                                const auto eddAddr=Account::instance()->get_addr({0,0,0}).get_address();
                                const auto issuerFea=Feature::Issuer(eddAddr);

                                auto addUnlcon=Unlock_Condition::Address(sender);
                                auto expirUnloc=Unlock_Condition::Expiration(maxB+100,eddAddr);
                                auto varretUlock=Unlock_Condition::Storage_Deposit_Return(eddAddr,0);
                                auto nftmetadata=Booking::create_new_bookings_metadata(varBooks);
                                auto metFea=Feature::Metadata(nftmetadata);
                                auto varNftOut= Output::NFT(0,{addUnlcon,varretUlock,expirUnloc},{},{issuerFea,metFea});
                                auto ret_amount=Client::get_deposit(varNftOut,info);

                                auto retUlock=Unlock_Condition::Storage_Deposit_Return(eddAddr,ret_amount);
                                auto NftOut= Output::NFT(0,{addUnlcon,retUlock,expirUnloc},{},{issuerFea,metFea});
                                NftOut->amount_=Client::get_deposit(NftOut,info)+payment_bundle.amount-price;

                                auto publish_bundle=Account::instance()->get_addr({0,0,0});
                                publish_bundle.consume_outputs(publish_node_outputs_->outs_);

                                const auto pubOut=get_publish_output(0);

                                if(publish_bundle.amount+payment_bundle.amount>=NftOut->amount_+Client::get_deposit(pubOut,info))
                                {
                                    pubOut->amount_=publish_bundle.amount+payment_bundle.amount-NftOut->amount_;
                                    auto Inputs_Commitment=Block::get_inputs_Commitment(payment_bundle.Inputs_hash+publish_bundle.Inputs_hash);


                                    pvector<const Output> the_outputs_{pubOut,NftOut};
                                    the_outputs_.insert( the_outputs_.end(), publish_bundle.ret_outputs.begin(), publish_bundle.ret_outputs.end());
                                    the_outputs_.insert( the_outputs_.end(), payment_bundle.ret_outputs.begin(), payment_bundle.ret_outputs.end());

                                    pvector<const Input> the_inputs_=payment_bundle.inputs;
                                    the_inputs_.insert( the_inputs_.end(), publish_bundle.inputs.begin(), publish_bundle.inputs.end());

                                    auto essence=Essence::Transaction(info->network_id_,the_inputs_,Inputs_Commitment,the_outputs_);

                                    payment_bundle.create_unlocks(essence->get_hash());
                                    publish_bundle.create_unlocks(essence->get_hash(),payment_bundle.unlocks.size());

                                    pvector<const Unlock> the_unlocks_=payment_bundle.unlocks;
                                    the_unlocks_.insert( the_unlocks_.end(), publish_bundle.unlocks.begin(), publish_bundle.unlocks.end());


                                    auto trpay=Payload::Transaction(essence,the_unlocks_);

                                    auto resp=Node_Conection::instance()->mqtt()->get_subscription("transactions/"+trpay->get_id().toHexString() +"/included-block");
                                    connect(resp,&ResponseMqtt::returned,this,[=](auto var){
                                        set_state(Ready);
                                        resp->deleteLater();
                                    });
                                    payments_.push_back(info->amount_json(payment_bundle.amount));
                                    emit paymentsChange();
                                    auto block_=Block(trpay);

                                    Node_Conection::instance()->rest()->send_block(block_);

                                    emit got_new_booking(varBooks);
                                    clean_state();
                                }
                                else
                                {
                                    emit notEnought(info->amount_json(NftOut->amount_+Client::get_deposit(pubOut,info) - publish_bundle.amount-payment_bundle.amount));
                                }

                            }


                        }
                    }


                }

                publish_node_outputs_->deleteLater();
                info->deleteLater();
            });
            Node_Conection::instance()->rest()->get_outputs<Output::Basic_typ>(publish_node_outputs_,
                                                                               "address="+Account::instance()->addr_bech32({0,0,0},info->bech32Hrp));
        });
    }
}


void Book_Server::handle_init_funds()
{

    auto info=Node_Conection::instance()->rest()->get_api_core_v2_info();
    QObject::connect(info,&Node_info::finished,this,[=]( ){

        auto node_outputs_=new Node_outputs();
        Node_Conection::instance()->rest()->get_outputs<Output::Basic_typ>(node_outputs_,"address="+Account::instance()->addr_bech32({0,0,0},info->bech32Hrp));

        QObject::connect(node_outputs_,&Node_outputs::finished,reciever,[=]( ){

            auto publish_bundle=Account::instance()->get_addr({0,0,0});

            publish_bundle.consume_outputs(node_outputs_->outs_);
            auto pubOut=get_publish_output(0);
            const auto min_output_pub=Client::get_deposit(pubOut,info);

            pubOut->amount_=publish_bundle.amount;
            auto Addrpub=Account::instance()->get_addr({0,0,0}).get_address();

            auto PubUnlcon=Unlock_Condition::Address(Addrpub);

            if(publish_bundle.amount>=min_output_pub)
            {
                auto Inputs_Commitment=Block::get_inputs_Commitment(publish_bundle.Inputs_hash);

                pvector<const Output> the_outputs_{pubOut};
                the_outputs_.insert( the_outputs_.end(), publish_bundle.ret_outputs.begin(), publish_bundle.ret_outputs.end());

                auto essence=Essence::Transaction(info->network_id_,publish_bundle.inputs,Inputs_Commitment,the_outputs_);

                publish_bundle.create_unlocks(essence->get_hash());

                auto trpay=Payload::Transaction(essence,publish_bundle.unlocks);

                auto block_=Block(trpay);
                auto resp=Node_Conection::instance()->mqtt()->get_subscription("transactions/"+trpay->get_id().toHexString() +"/included-block");
                connect(resp,&ResponseMqtt::returned,this,[=](auto var){
                    set_state(Ready);
                    resp->deleteLater();
                });
                set_state(Publishing);
                Node_Conection::instance()->rest()->send_block(block_);
                started=true;
            }

            info->deleteLater();
            node_outputs_->deleteLater();
        });
    });


}


void Book_Server::try_to_open(void)
{
    static ResponseMqtt* resp=nullptr;
    if(resp)resp->deleteLater();
    auto info=Node_Conection::instance()->rest()->get_api_core_v2_info();
    connect(info,&Node_info::finished,this,[=]( ){

        quint32 r1 = QRandomGenerator::global()->generate();
        quint32 r2 = QRandomGenerator::global()->generate();


        const auto address=Account::instance()->addr_bech32({100,r1,r2},info->bech32Hrp);

        resp=Node_Conection::instance()->mqtt()->
               get_outputs_unlock_condition_address("address/"+address);
        QObject::connect(resp,&ResponseMqtt::returned,reciever,[=](QJsonValue data){
            check_nft_to_open(Node_output(data));
            resp->deleteLater();
            resp=nullptr;
        });
        emit nftAddress(address);
        info->deleteLater();
    });
}
QByteArray Book_Server::serialize_state(void)const
{
    QJsonObject var;
    QJsonArray bookings;

    for(const auto& v: books_)
    {
        bookings.append(v["start"]);
        bookings.append(v["finish"]);
    }
    var.insert("bookings",bookings);
    var.insert("price_per_hour",QString::number(price_per_hour_));
    var.insert("pay_to",Account::instance()->addr({0,0,1}));
#if defined(RPI_SERVER)
    if(m_GeoCoord.isValid())var.insert("coord",QJsonArray({m_GeoCoord.longitude(),m_GeoCoord.latitude()}));
#endif
    auto state = QJsonDocument(var);
    return state.toJson();
}
void Book_Server::deserialize_state(const QByteArray &state)
{
    const auto var=QJsonDocument::fromJson(state).object();
    if(!var["bookings"].isUndefined()&&var["bookings"].isArray())
    {
        const auto bookarray=var["bookings"].toArray();
        const auto vec=Booking::from_Array(bookarray);
        for(const auto& v:vec)
        {
            books_.insert(v);

        }
        emit got_new_booking(bookarray);
        started=true;
    }

}
