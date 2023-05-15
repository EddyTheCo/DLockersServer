#include"bserver.hpp"
#include <QCryptographicHash>
#include <QDebug>
#include<QJsonDocument>
#include<QTimer>
#include <QRandomGenerator>
using namespace qiota::qblocks;

using namespace qiota;
using namespace qcrypto;

void Book_Server::init()
{
    auto info=Node_Conection::rest_client->get_api_core_v2_info();
    QObject::connect(info,&Node_info::finished,reciever,[=]( ){

        auto resp2=Node_Conection::mqtt_client->
                get_outputs_unlock_condition_address("address/"+Account::addr_bech32({0,0,0},info->bech32Hrp));
        connect(resp2,&ResponseMqtt::returned,reciever,[=](QJsonValue data)
        {
            if(!started)
            {
                this->handle_init_funds();
            }
            checkFunds({Node_output(data)});

        });
        this->handle_init_funds();

        auto resp=Node_Conection::mqtt_client->
                get_outputs_unlock_condition_address("address/"+Account::addr_bech32({0,0,1},info->bech32Hrp));
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
    auto info=Node_Conection::rest_client->get_api_core_v2_info();
    connect(info,&Node_info::finished,reciever,[=]( ){
        setServerId(Account::addr_bech32({0,0,0},info->bech32Hrp));
        auto node_outputs_=new Node_outputs();
        connect(node_outputs_,&Node_outputs::finished,reciever,[=]( ){
            check_state_output(node_outputs_->outs_);
            init();
            node_outputs_->deleteLater();
        });
        Node_Conection::rest_client->get_outputs<Output::Basic_typ>(node_outputs_,"address="+
                                                                    Account::addr_bech32({0,0,0},info->bech32Hrp)+
                                                                    "&hasStorageDepositReturn=false&hasTimelock=false&hasExpiration=false&sender="+
                                                                    Account::addr_bech32({0,0,0},info->bech32Hrp)+"&tag="+fl_array<quint8>("state").toHexString());
        auto node_outputs=new Node_outputs();
        connect(node_outputs,&Node_outputs::finished,reciever,[=]( ){
            checkFunds(node_outputs->outs_);
            node_outputs->deleteLater();
        });
        Node_Conection::rest_client->get_outputs<Output::Basic_typ>(node_outputs,"address="+
                                                                    Account::addr_bech32({0,0,0},info->bech32Hrp));

        info->deleteLater();
    });

}
void Book_Server::restart(void)
{

    if(Node_Conection::state()==Node_Conection::Connected)
    {
        if(reciever)reciever->deleteLater();
        reciever=new QObject(this);
        setFunds(0);
        setminFunds(0);
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
        auto bundle= Account::get_addr({0,0,0});
        bundle.consume_outputs(var);
        qDebug()<<"amount:"<<bundle.amount;
        if(bundle.amount)
        {
            total+=bundle.amount;
            total_funds.insert(v.metadata().outputid_.toHexString(),bundle.amount);
            auto resp=Node_Conection::mqtt_client->get_outputs_outputId(v.metadata().outputid_.toHexString());
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
                auto resp=Node_Conection::mqtt_client->get_outputs_outputId(v.metadata().outputid_.toHexString());
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
        auto info=Node_Conection::rest_client->get_api_core_v2_info();
        QObject::connect(info,&Node_info::finished,reciever,[=]( ){
            funds_json=info->amount_json(funds_);
            emit fundsChanged();
            info->deleteLater();
        });
    }
}
void Book_Server::setminFunds(quint64 funds_m){
    auto info=Node_Conection::rest_client->get_api_core_v2_info();
    QObject::connect(info,&Node_info::finished,reciever,[=]( ){
        minfunds_json=info->amount_json(funds_m);
        emit minfundsChanged();
        info->deleteLater();
    });

}
Book_Server::Book_Server(QObject *parent):QObject(parent),price_per_hour_(10000),state_(Ready),reciever(nullptr),started(false),
    open(false)
{
    restart();
}

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
    const auto eddAddr=Account::get_addr({0,0,0}).get_address();
    auto sendFea=Feature::Sender(eddAddr);
    auto tagFea=Feature::Tag(tag);

    auto metFea=Feature::Metadata(state);

    auto addUnlcon=Unlock_Condition::Address(eddAddr);
    return Output::Basic(amount,{addUnlcon},{},{sendFea,metFea,tagFea});
}
void Book_Server::check_nft_to_open(Node_output node_out)
{

    if(node_out.output()->type()==Output::NFT_typ)
    {
        const auto output=node_out.output();
        const auto issuerfeature=output->get_immutable_feature_(Feature::Issuer_typ);
        if(issuerfeature)
        {
            const auto issuer=std::static_pointer_cast<const Issuer_Feature>(issuerfeature)->issuer()->addr();
            if(issuer==Account::get_addr({0,0,0}).get_address()->addr())
            {

                const auto metfeature=output->get_immutable_feature_(Feature::Metadata_typ);
                if(metfeature)
                {
                    const auto metadata=std::static_pointer_cast<const Metadata_Feature>(metfeature)->data();

                    auto book=Booking::get_new_booking_from_metadata(metadata);

                    auto now=QDateTime::currentDateTime().toSecsSinceEpoch();



                    if(book["finish"].toInteger()>=now&&book["start"].toInteger()<=now)
                    {
                        open=true;
                        emit openChanged();

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
        auto info=Node_Conection::rest_client->get_api_core_v2_info();
        connect(info,&Node_info::finished,this,[=]( ){

            auto publish_node_outputs_= new Node_outputs();

            connect(publish_node_outputs_,&Node_outputs::finished,this,[=]( ){
                const auto basic_output_=node_out.output();
                const auto metfeature=basic_output_->get_feature_(Feature::Metadata_typ);
                if(metfeature)
                {
                    auto metadata_feature=std::static_pointer_cast<const Metadata_Feature>(metfeature);
                    auto metadata=metadata_feature->data();

                    auto new_book=Booking::get_new_booking_from_metadata(metadata);
                    const auto now=QDateTime::currentDateTime().toSecsSinceEpoch();
                    const auto sendfeature=basic_output_->get_feature_(Feature::Sender_typ);
                    if(sendfeature)
                    {
                        auto sender=std::static_pointer_cast<const Sender_Feature>(sendfeature)->sender();

                        if(!new_book.isEmpty()&&new_book["finish"].toInteger()>=now)
                        {

                            auto output=std::vector<Node_output>{node_out};
                            auto payment_bundle=Account::get_addr({0,0,1});
                            payment_bundle.consume_outputs({output});


                            const auto eddAddr=Account::get_addr({0,0,0}).get_address();
                            const auto issuerFea=Feature::Issuer(eddAddr);

                            auto addUnlcon=Unlock_Condition::Address(sender);
                            auto nftmetadata=Booking::create_new_bookings_metadata(new_book);
                            auto metFea=Feature::Metadata(nftmetadata);
                            auto NftOut= Output::NFT(0,{addUnlcon},{},{issuerFea,metFea});
                            NftOut->amount_=Client::get_deposit(NftOut,info);

                            if(payment_bundle.amount>=new_book.calculate_price(price_per_hour_)+NftOut->amount_)
                            {
                                auto pair=books_.insert(new_book);
                                if(pair.second)
                                {

                                    auto publish_bundle=Account::get_addr({0,0,0});
                                    publish_bundle.consume_outputs(publish_node_outputs_->outs_);

                                    auto Inputs_Commitment=Block::get_inputs_Commitment(payment_bundle.Inputs_hash+publish_bundle.Inputs_hash);

                                    const auto pubOut=get_publish_output(publish_bundle.amount+payment_bundle.amount-NftOut->amount_);
                                    pvector<const Output> the_outputs_{pubOut,NftOut};
                                    the_outputs_.insert( the_outputs_.end(), publish_bundle.ret_outputs.begin(), publish_bundle.ret_outputs.end());
                                    the_outputs_.insert( the_outputs_.end(), payment_bundle.ret_outputs.begin(), payment_bundle.ret_outputs.end());

                                    pvector<const Input> the_inputs_=payment_bundle.inputs;
                                    the_inputs_.insert( the_inputs_.end(), publish_bundle.inputs.begin(), publish_bundle.inputs.end());

                                    auto essence=Essence::Transaction(info->network_id_,the_inputs_,Inputs_Commitment,the_outputs_);

                                    payment_bundle.create_unlocks(essence->get_hash());
                                    publish_bundle.create_unlocks(essence->get_hash());

                                    pvector<const Unlock> the_unlocks_=payment_bundle.unlocks;
                                    the_unlocks_.insert( the_unlocks_.end(), publish_bundle.unlocks.begin(), publish_bundle.unlocks.end());


                                    auto trpay=Payload::Transaction(essence,the_unlocks_);

                                    auto resp=Node_Conection::mqtt_client->get_subscription("transactions/"+trpay->get_id().toHexString() +"/included-block");
                                    connect(resp,&ResponseMqtt::returned,this,[=](auto var){
                                        set_state(Ready);
                                        resp->deleteLater();
                                    });
                                    payments_.push_back(info->amount_json(payment_bundle.amount));
                                    emit paymentsChange();
                                    auto block_=Block(trpay);

                                    Node_Conection::rest_client->send_block(block_);
                                    emit got_new_booking(new_book);
                                    clean_state();
                                }

                            }
                        }
                    }


                }

                publish_node_outputs_->deleteLater();
                info->deleteLater();
            });
            Node_Conection::rest_client->get_outputs<Output::Basic_typ>(publish_node_outputs_,
                                                                        "address="+Account::addr_bech32({0,0,0},info->bech32Hrp));
        });
    }
}


void Book_Server::handle_init_funds()
{

    auto info=Node_Conection::rest_client->get_api_core_v2_info();
    QObject::connect(info,&Node_info::finished,this,[=]( ){

        auto node_outputs_=new Node_outputs();
        Node_Conection::rest_client->get_outputs<Output::Basic_typ>(node_outputs_,"address="+Account::addr_bech32({0,0,0},info->bech32Hrp));

        QObject::connect(node_outputs_,&Node_outputs::finished,reciever,[=]( ){

            auto publish_bundle=Account::get_addr({0,0,0});

            publish_bundle.consume_outputs(node_outputs_->outs_);
            auto pubOut=get_publish_output(0);
            const auto min_output_pub=Client::get_deposit(pubOut,info);
            setminFunds(min_output_pub);
            pubOut->amount_=publish_bundle.amount;
            auto Addrpub=Account::get_addr({0,0,0}).get_address();

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
                set_state(Publishing);
                Node_Conection::rest_client->send_block(block_);
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
    auto info=Node_Conection::rest_client->get_api_core_v2_info();
    connect(info,&Node_info::finished,this,[=]( ){

        quint32 r1 = QRandomGenerator::global()->generate();
        quint32 r2 = QRandomGenerator::global()->generate();


        const auto address=Account::addr_bech32({100,r1,r2},info->bech32Hrp);

        resp=Node_Conection::mqtt_client->
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
        bookings.append(v);
    }
    var.insert("bookings",bookings);
    var.insert("price_per_hour",QString::number(price_per_hour_));
    var.insert("pay_to",Account::addr({0,0,1}));
    auto state = QJsonDocument(var);
    return state.toJson();
}
void Book_Server::deserialize_state(const QByteArray &state)
{
    const auto var=QJsonDocument::fromJson(state).object();
    if(!var["bookings"].isUndefined()&&var["bookings"].isArray())
    {
        const auto bookarray=var["bookings"].toArray();
        for(const auto& v:bookarray)
        {
            books_.insert(v.toObject());
            emit got_new_booking(v);
        }
        started=true;
    }

}
