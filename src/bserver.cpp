#include"bserver.hpp"
#include <QCryptographicHash>
#include <QDebug>
#include<QJsonDocument>
#include<QTimer>
using namespace qiota::qblocks;

using namespace qiota;
using namespace qcrypto;
void Book_Server::wait_new_state(void)
{
    auto info=Node_Conection::rest_client->get_api_core_v2_info();
    QObject::connect(info,&Node_info::finished,reciever,[=]( ){
        auto resp=Node_Conection::mqtt_client->
                get_outputs_unlock_condition_address("address/"+Account::addr_bech32({0,0,0},info->bech32Hrp));
        connect(resp,&ResponseMqtt::returned,reciever,[=](QJsonValue data)
        {
            const auto node_out=Node_output(data);
            qDebug()<<"got_new_state";
            if(node_out.output()->type_m==qblocks::Output::Basic_typ)
            {
                qDebug()<<"is basic";
                const auto basic_output_=std::dynamic_pointer_cast<qblocks::Basic_Output>(node_out.output());
                const auto sendfea=basic_output_->get_feature_(qblocks::Feature::Sender_typ);
                if(sendfea&&std::dynamic_pointer_cast<qblocks::Sender_Feature>(sendfea)->sender()->addr()==
                        Account::get_addr({0,0,0}).get_address<qblocks::Address::Ed25519_typ>())
                {
                    qDebug()<<"sender is me";
                    const auto tagfeau=basic_output_->get_feature_(qblocks::Feature::Tag_typ);
                    if(tagfeau&&std::dynamic_pointer_cast<qblocks::Tag_Feature>(tagfeau)->tag()==fl_array<quint8>("state"))
                    {
                        qDebug()<<"tag is state";
                        set_state(Ready);
                        resp->deleteLater();
                    }
                }
            }

        });

        QTimer::singleShot(60000,resp,[=](){this->set_state(Ready); resp->deleteLater();});
        info->deleteLater();
    });
}
void Book_Server::init()
{

    auto info=Node_Conection::rest_client->get_api_core_v2_info();
    QObject::connect(info,&Node_info::finished,reciever,[=]( ){

        auto resp2=Node_Conection::mqtt_client->
                get_outputs_unlock_condition_address("address/"+Account::addr_bech32({0,0,0},info->bech32Hrp));
        connect(resp2,&ResponseMqtt::returned,reciever,[=](QJsonValue data)
        {
            qDebug()<<"got state output started:"<<started;
            if(!started)this->handle_init_funds();
        });
        this->handle_init_funds();

        auto resp=Node_Conection::mqtt_client->
                get_outputs_unlock_condition_address("address/"+Account::addr_bech32({0,0,1},info->bech32Hrp));
        QObject::connect(resp,&ResponseMqtt::returned,reciever,[=](QJsonValue data){
            qDebug()<<"handle_new_book:"<<state_;
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
        const auto basic_output_=std::dynamic_pointer_cast<qblocks::Basic_Output>(node_output_s.front().output());
        const auto metfeau=basic_output_->get_feature_(qblocks::Feature::Metadata_typ);
        if(metfeau)
        {
            auto metadata_feature=std::dynamic_pointer_cast<qblocks::Metadata_Feature>(metfeau);
            auto metadata=metadata_feature->data();
            auto server_data=Booking::deserialize_state(metadata);

            if(std::get<2>(server_data)!=QByteArray(32,0))
            {
                price_per_hour_=std::get<1>(server_data);
                for(const auto& new_booking:std::get<0>(server_data))
                {
                    emit got_new_booking(new_booking);
                }

            }

        }
    }
    emit finishRestart();
}
void Book_Server::get_restart_state(void)
{
    auto info=Node_Conection::rest_client->get_api_core_v2_info();
    QObject::connect(info,&Node_info::finished,reciever,[=]( ){

        auto node_outputs_=new Node_outputs();
        Node_Conection::rest_client->get_basic_outputs(node_outputs_,"address="+
                                                       Account::addr_bech32({0,0,0},info->bech32Hrp)+
                                                       "&hasStorageDepositReturn=false&hasTimelock=false&hasExpiration=false&sender="+
                                                       Account::addr_bech32({0,0,0},info->bech32Hrp)+"&tag="+fl_array<quint8>("state").toHexString());
        QObject::connect(node_outputs_,&Node_outputs::finished,reciever,[=]( ){
            check_state_output(node_outputs_->outs_);
            node_outputs_->deleteLater();
        });
        info->deleteLater();
    });

}
void Book_Server::init_min_amount_funds(void)
{

    if(Node_Conection::state()==Node_Conection::Connected)
    {
        if(reciever)reciever->deleteLater();
        reciever=new QObject(this);
        get_restart_state();

        connect(this,&Book_Server::stateChanged,reciever,[=](){
            if(state_==Ready)
            {
                while (!queue.empty()) {
                    handle_new_book(queue.front());
                    queue.pop();
                }
            }
        });
    }

}

Book_Server::Book_Server():price_per_hour_(10000),state_(Ready),reciever(nullptr),started(false)
{
    connect(this,&Book_Server::finishRestart,this,&Book_Server::init);

    init_min_amount_funds();



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
    const auto state=Booking::serialize_state(books_,price_per_hour_,Account::get_addr({0,0,1}).get_hash());
    const auto eddAddr=std::shared_ptr<Address>(new Ed25519_Address(Account::get_addr({0,0,0}).get_hash()));
    auto sendFea=std::shared_ptr<qblocks::Feature>(new Sender_Feature(eddAddr));
    auto tagFea=std::shared_ptr<qblocks::Feature>(new Tag_Feature(tag));
    auto metFea=std::shared_ptr<qblocks::Feature>(new Metadata_Feature(state));

    auto addUnlcon=std::shared_ptr<qblocks::Unlock_Condition>(new Address_Unlock_Condition(eddAddr));

    return std::shared_ptr<qblocks::Output>(new Basic_Output(amount,{addUnlcon},{sendFea,metFea,tagFea},{}));
}
void Book_Server::handle_new_book(Node_output node_out)
{
    qDebug()<<" Book_Server::handle_new_book";
    if(node_out.output()->type_m==qblocks::Output::Basic_typ)
    {
        auto info=Node_Conection::rest_client->get_api_core_v2_info();
        QObject::connect(info,&Node_info::finished,this,[=]( ){

            auto publish_node_outputs_= new Node_outputs();
            Node_Conection::rest_client->get_basic_outputs(publish_node_outputs_,
                                                           "address="+Account::addr_bech32({0,0,0},info->bech32Hrp));

            QObject::connect(publish_node_outputs_,&Node_outputs::finished,this,[=]( ){
                const auto basic_output_=std::dynamic_pointer_cast<qblocks::Basic_Output>(node_out.output());
                const auto metfeature=basic_output_->get_feature_(qblocks::Feature::Metadata_typ);
                if(metfeature)
                {
                    auto metadata_feature=std::dynamic_pointer_cast<qblocks::Metadata_Feature>(metfeature);
                    auto metadata=metadata_feature->data();

                    auto new_book=Booking::get_new_booking_from_metadata(metadata);

                    if(new_book.finish()>=QDateTime::currentDateTime()&&new_book.start().daysTo(new_book.finish())<7)
                    {
                        c_array Inputs_Commitments;
                        quint64 amount=0;
                        std::vector<std::shared_ptr<qblocks::Output>> ret_outputs;
                        std::vector<std::shared_ptr<qblocks::Input>> inputs;

                        auto output=std::vector<Node_output>{node_out};
                        auto payment_bundle=Account::get_addr({0,0,1});
                        payment_bundle.consume_outputs({output},0,Inputs_Commitments,amount,ret_outputs,inputs);

                        qDebug()<<"amount:"<<amount<<" price:"<<new_book.calculate_price(price_per_hour_);
                        if(amount>=new_book.calculate_price(price_per_hour_))
                        {
                            auto pair=books_.insert(new_book);
                            if(pair.second)
                            {

                                qDebug()<<"inserted";
                                auto pbundle=Account::get_addr({0,0,0});
                                pbundle.consume_outputs(publish_node_outputs_->outs_,0,Inputs_Commitments,amount,ret_outputs,inputs);

                                auto Inputs_Commitment=c_array(QCryptographicHash::hash(Inputs_Commitments, QCryptographicHash::Blake2b_256));

                                const auto pubOut=get_publish_output(amount);
                                std::vector<std::shared_ptr<qblocks::Output>> the_outputs_{pubOut};
                                the_outputs_.insert( the_outputs_.end(), ret_outputs.begin(), ret_outputs.end());
                                auto essence=std::shared_ptr<qblocks::Essence>(
                                            new Transaction_Essence(info->network_id_,inputs,Inputs_Commitment,the_outputs_,nullptr));
                                c_array serializedEssence;
                                serializedEssence.from_object<Essence>(*essence);
                                auto essence_hash=QCryptographicHash::hash(serializedEssence, QCryptographicHash::Blake2b_256);
                                std::vector<std::shared_ptr<qblocks::Unlock>> unlocks;

                                payment_bundle.create_unlocks<qblocks::Reference_Unlock>(essence_hash,unlocks);

                                pbundle.create_unlocks<qblocks::Reference_Unlock>(essence_hash,unlocks);

                                auto trpay=std::shared_ptr<qblocks::Payload>(new Transaction_Payload(essence,unlocks));
                                auto block_=Block(trpay);
                                set_state(Publishing);
                                wait_new_state();
                                Node_Conection::rest_client->send_block(block_);
                                emit got_new_booking(new_book);
                                clean_state();


                            }

                        }
                    }

                }

                publish_node_outputs_->deleteLater();
                info->deleteLater();
            });

        });
    }
}


void Book_Server::handle_init_funds()
{
    qDebug()<<"handle_init_funds";
    auto info=Node_Conection::rest_client->get_api_core_v2_info();
    QObject::connect(info,&Node_info::finished,this,[=]( ){

        auto node_outputs_=new Node_outputs();
        Node_Conection::rest_client->get_basic_outputs(node_outputs_,"address="+Account::addr_bech32({0,0,0},info->bech32Hrp));

        QObject::connect(node_outputs_,&Node_outputs::finished,reciever,[=]( ){

            auto publish_bundle=Account::get_addr({0,0,0});
            c_array Inputs_Commitments;
            quint64 amount=0;
            std::vector<std::shared_ptr<qblocks::Output>> ret_outputs;
            std::vector<std::shared_ptr<qblocks::Input>> inputs;

            publish_bundle.consume_outputs(node_outputs_->outs_,0,Inputs_Commitments,amount,ret_outputs,inputs);
            auto pubOut=get_publish_output(0);
            const auto min_output_pub=pubOut->min_deposit_of_output(info->vByteFactorKey,info->vByteFactorData,info->vByteCost);
            pubOut->amount_=amount;
            auto Addrpub=std::shared_ptr<Address>(new Ed25519_Address(Account::get_addr({0,0,0}).get_hash()));
            auto PubUnlcon=std::shared_ptr<qblocks::Unlock_Condition>(new Address_Unlock_Condition(Addrpub));
            qDebug()<<"amount:"<<amount<<" min_output_pub:"<<min_output_pub;
            if(amount>=min_output_pub)
            {

                auto Inputs_Commitment=c_array(QCryptographicHash::hash(Inputs_Commitments, QCryptographicHash::Blake2b_256));
                std::vector<std::shared_ptr<qblocks::Output>> the_outputs_{pubOut};
                the_outputs_.insert( the_outputs_.end(), ret_outputs.begin(), ret_outputs.end());
                auto essence=std::shared_ptr<qblocks::Essence>(
                            new Transaction_Essence(info->network_id_,inputs,Inputs_Commitment,the_outputs_,nullptr));

                c_array serializedEssence;
                serializedEssence.from_object<Essence>(*essence);

                auto essence_hash=QCryptographicHash::hash(serializedEssence, QCryptographicHash::Blake2b_256);
                std::vector<std::shared_ptr<qblocks::Unlock>> unlocks;
                publish_bundle.create_unlocks<qblocks::Reference_Unlock>(essence_hash,unlocks);

                auto trpay=std::shared_ptr<qblocks::Payload>(new Transaction_Payload(essence,{unlocks}));
                auto block_=Block(trpay);
                set_state(Publishing);
                wait_new_state();
                started=true;
                Node_Conection::rest_client->send_block(block_);
                set_transfer_funds(0);

            }
            else
            {
                qDebug()<<"set_transfer_funds:"<<min_output_pub-amount;
                set_transfer_funds(min_output_pub-amount);
            }
            info->deleteLater();
            node_outputs_->deleteLater();
        });
    });


}


bool Book_Server::try_to_open(QString code)
{

    clean_state();

    auto now=QDateTime::currentDateTime();
    if(books_.size()&&(books_.begin()->start()<=now)&&(books_.begin()->finish()>=now)&&books_.begin()->verify_code_str(code))
    {
        open=true;
        return true;
        emit open_changed(open);
    }

    return false;
}
