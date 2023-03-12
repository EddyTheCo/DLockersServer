#include"bserver.hpp"
#include <QCryptographicHash>
#include <QDebug>
#include<QJsonDocument>

using namespace qiota::qblocks;

using namespace qiota;
using namespace qcrypto;

void Book_Server::init_min_amount_funds(void)
{

    if(Node_Conection::state()==Node_Conection::Connected)
    {
        if(reciever)reciever->deleteLater();
        reciever=new QObject(this);
        auto info=Node_Conection::rest_client->get_api_core_v2_info();
        QObject::connect(info,&Node_info::finished,this,[=]( ){
            cleanConsolidation();

            auto resp2=Node_Conection::mqtt_client->
                    get_outputs_unlock_condition_address("address/"+Account::addr_bech32({0,0,0},info->bech32Hrp));
            connect(resp2,&ResponseMqtt::returned,reciever,[=](QJsonValue data)
            {
                this->handle_init_funds(resp2);
            });
            this->handle_init_funds(resp2);
            auto resp=Node_Conection::mqtt_client->
                    get_outputs_unlock_condition_address("address/"+Account::addr_bech32({0,0,1},info->bech32Hrp));
            QObject::connect(resp,&ResponseMqtt::returned,reciever,[=](QJsonValue data){
                handle_new_book(Node_output(data));
            });
            info->deleteLater();
        });


    }

}

Book_Server::Book_Server():price_per_hour_(10000),publishing_(false),consolidation_index(0),
    cons_step_index(10),reciever(nullptr)
{
    init_min_amount_funds();

}
void Book_Server::cleanConsolidation()const
{

    auto info=Node_Conection::rest_client->get_api_core_v2_info();
    QObject::connect(info,&Node_info::finished,this,[=]( ){

        for(quint32 i=consolidation_index;i<consolidation_index+cons_step_index;i++)
        {
            auto node_outputs_=new Node_outputs();
            Node_Conection::rest_client->get_basic_outputs(node_outputs_,"address="+Account::addr_bech32({0,1,i},info->bech32Hrp));

            QObject::connect(node_outputs_,&Node_outputs::finished,reciever,[=]( ){
                auto publish_bundle=Account::get_addr({0,0,0});
                auto cons_bundle=Account::get_addr({0,1,i});

                c_array Inputs_Commitments;
                quint64 amount=0;
                std::vector<std::shared_ptr<qblocks::Output>> ret_outputs;
                std::vector<std::shared_ptr<qblocks::Input>> inputs;

                cons_bundle.consume_outputs(node_outputs_->outs_,0,Inputs_Commitments,amount,ret_outputs,inputs);

                if(amount)
                {
                    auto Inputs_Commitment=c_array(QCryptographicHash::hash(Inputs_Commitments, QCryptographicHash::Blake2b_256));
                    auto Addrpub=std::shared_ptr<Address>(new Ed25519_Address(publish_bundle.get_hash()));
                    auto PubUnlcon=std::shared_ptr<qblocks::Unlock_Condition>(new Address_Unlock_Condition(Addrpub));
                    const auto pubOutput= std::shared_ptr<qblocks::Output> (new Basic_Output(amount,{PubUnlcon},{},{}));
                    std::vector<std::shared_ptr<qblocks::Output>> the_outputs_{pubOutput};
                    the_outputs_.insert( the_outputs_.end(), ret_outputs.begin(), ret_outputs.end());
                    auto essence=std::shared_ptr<qblocks::Essence>(
                                new Transaction_Essence(info->network_id_,inputs,Inputs_Commitment,the_outputs_,nullptr));


                    c_array serializedEssence;
                    serializedEssence.from_object<Essence>(*essence);

                    auto essence_hash=QCryptographicHash::hash(serializedEssence, QCryptographicHash::Blake2b_256);
                    std::vector<std::shared_ptr<qblocks::Unlock>> unlocks;
                    cons_bundle.create_unlocks<qblocks::Reference_Unlock>(essence_hash,unlocks);

                    auto trpay=std::shared_ptr<qblocks::Payload>(new Transaction_Payload(essence,{unlocks}));
                    auto block_=Block(trpay);
                    Node_Conection::rest_client->send_block(block_);

                }

            });
        }

    });
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
qblocks::fl_array<quint16> Book_Server::get_state_metadata(void)const
{
    qblocks::fl_array<quint16> var;
    auto buffer=QDataStream(&var,QIODevice::WriteOnly | QIODevice::Append);
    buffer.setByteOrder(QDataStream::LittleEndian);
    buffer<<static_cast<quint16>(books_.size());
    for(auto& v:books_)
    {
        v.serialize(buffer,0);
    }
    buffer<<price_per_hour_;
    buffer.writeRawData(Account::get_addr({0,0,1}).get_hash(),32);
    return var;

}

Booking Book_Server::get_new_booking_from_metadata(qblocks::fl_array<quint16> metadata)const
{
    if(metadata.size()==48)
    {
        auto buffer=QDataStream(&metadata,QIODevice::ReadOnly);
        buffer.setByteOrder(QDataStream::LittleEndian);
        return Booking(buffer,1);
    }
    return Booking(QDateTime::fromMSecsSinceEpoch(0),QDateTime::fromMSecsSinceEpoch(0));
}

std::shared_ptr<qblocks::Output> Book_Server::get_publish_output(const quint64 &amount)const
{

    const fl_array<quint8> tag("state");
    const auto state=get_state_metadata();
    const auto eddAddr=std::shared_ptr<Address>(new Ed25519_Address(Account::get_addr({0,0,0}).get_hash()));
    auto sendFea=std::shared_ptr<qblocks::Feature>(new Sender_Feature(eddAddr));
    auto tagFea=std::shared_ptr<qblocks::Feature>(new Tag_Feature(tag));
    auto metFea=std::shared_ptr<qblocks::Feature>(new Metadata_Feature(state));

    auto addUnlcon=std::shared_ptr<qblocks::Unlock_Condition>(new Address_Unlock_Condition(eddAddr));

    return std::shared_ptr<qblocks::Output>(new Basic_Output(amount,{addUnlcon},{sendFea,metFea,tagFea},{}));
}
void Book_Server::handle_new_book(Node_output node_out)
{

    if(node_out.output()->type_m==qblocks::Output::Basic_typ)
    {
        auto info=Node_Conection::rest_client->get_api_core_v2_info();
        QObject::connect(info,&Node_info::finished,this,[=]( ){

            auto cons_node_outputs_= new Node_outputs();
            const auto cons_bundle=Account::get_addr({0,1,consolidation_index});
            Node_Conection::rest_client->get_basic_outputs(cons_node_outputs_,
                                                           "address="+cons_bundle.get_address_bech32<qblocks::Address::Ed25519_typ>(info->bech32Hrp));
            connect(cons_node_outputs_,&Node_outputs::finished,this,[=]( ){
                auto publish_node_outputs_= new Node_outputs();
                const auto publish_bundle=Account::get_addr({0,0,0});
                Node_Conection::rest_client->get_basic_outputs(publish_node_outputs_,
                                                               "address="+publish_bundle.get_address_bech32<qblocks::Address::Ed25519_typ>(info->bech32Hrp));

                QObject::connect(publish_node_outputs_,&Node_outputs::finished,this,[=]( ){
                    const auto basic_output_=std::dynamic_pointer_cast<qblocks::Basic_Output>(node_out.output());
                    const auto metfeature=basic_output_->get_feature_(qblocks::Feature::Metadata_typ);
                    if(metfeature)
                    {
                        auto metadata_feature=std::dynamic_pointer_cast<qblocks::Metadata_Feature>(metfeature);
                        auto metadata=metadata_feature->data();

                        auto new_book=get_new_booking_from_metadata(metadata);

                        if(new_book.finish()>=QDateTime::currentDateTime()&&new_book.start().daysTo(new_book.finish())<7)
                        {
                            c_array Inputs_Commitments;
                            quint64 amount=0;
                            std::vector<std::shared_ptr<qblocks::Output>> ret_outputs;
                            std::vector<std::shared_ptr<qblocks::Input>> inputs;

                            auto output=std::vector<Node_output>{node_out};
                            auto payment_bundle=Account::get_addr({0,0,1});
                            payment_bundle.consume_outputs({output},0,Inputs_Commitments,amount,ret_outputs,inputs);

                            if(amount>=new_book.calculate_price(price_per_hour_))
                            {
                                auto pair=books_.insert(new_book);
                                if(pair.second)
                                {
                                    consolidation_index++;
                                    auto cbundle=cons_bundle;
                                    auto pbundle=publish_bundle;
                                    cbundle.consume_outputs(cons_node_outputs_->outs_,0,Inputs_Commitments,amount,ret_outputs,inputs);
                                    pbundle.consume_outputs(publish_node_outputs_->outs_,0,Inputs_Commitments,amount,ret_outputs,inputs);

                                    auto Inputs_Commitment=c_array(QCryptographicHash::hash(Inputs_Commitments, QCryptographicHash::Blake2b_256));

                                    auto consAddr=std::shared_ptr<Address>(new Ed25519_Address(Account::get_addr({0,1,consolidation_index+cons_step_index}).get_hash()));
                                    auto consaddrUnlcon=std::shared_ptr<qblocks::Unlock_Condition>(new Address_Unlock_Condition(consAddr));
                                    const auto min_deposit= Basic_Output(0,{consaddrUnlcon},{},{})
                                            .min_deposit_of_output(info->vByteFactorKey,info->vByteFactorData,info->vByteCost);

                                    const auto consOut= std::shared_ptr<qblocks::Output>(new Basic_Output(min_deposit,{consaddrUnlcon},{},{}));

                                    const auto pubOut=get_publish_output(amount-min_deposit);
                                    std::vector<std::shared_ptr<qblocks::Output>> the_outputs_{consOut,pubOut};
                                    the_outputs_.insert( the_outputs_.end(), ret_outputs.begin(), ret_outputs.end());
                                    auto essence=std::shared_ptr<qblocks::Essence>(
                                                new Transaction_Essence(info->network_id_,inputs,Inputs_Commitment,the_outputs_,nullptr));
                                    c_array serializedEssence;
                                    serializedEssence.from_object<Essence>(*essence);
                                    auto essence_hash=QCryptographicHash::hash(serializedEssence, QCryptographicHash::Blake2b_256);
                                    std::vector<std::shared_ptr<qblocks::Unlock>> unlocks;

                                    payment_bundle.create_unlocks<qblocks::Reference_Unlock>(essence_hash,unlocks);
                                    cons_bundle.create_unlocks<qblocks::Reference_Unlock>(essence_hash,unlocks);
                                    publish_bundle.create_unlocks<qblocks::Reference_Unlock>(essence_hash,unlocks);

                                    auto trpay=std::shared_ptr<qblocks::Payload>(new Transaction_Payload(essence,unlocks));
                                    auto block_=Block(trpay);
                                    Node_Conection::rest_client->send_block(block_);
                                    emit got_new_booking(new_book);
                                    clean_state();

                                }

                            }
                        }

                    }
                    cons_node_outputs_->deleteLater();
                    publish_node_outputs_->deleteLater();
                    info->deleteLater();
                });


            });


        });


    }
}


void Book_Server::handle_init_funds(ResponseMqtt* respo)
{
    if(respo)
    {
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
                pubOut->amount_=min_output_pub;
                auto Addrpub=std::shared_ptr<Address>(new Ed25519_Address(Account::get_addr({0,0,0}).get_hash()));
                auto PubUnlcon=std::shared_ptr<qblocks::Unlock_Condition>(new Address_Unlock_Condition(Addrpub));
                const auto min_deposit_pay= Basic_Output(0,{PubUnlcon},{},{})
                        .min_deposit_of_output(info->vByteFactorKey,info->vByteFactorData,info->vByteCost);
                min_amount_funds_=min_output_pub+cons_step_index*min_deposit_pay;
                qDebug()<<"min_amount_funds_:"<<min_amount_funds_;
                qDebug()<<"amount:"<<amount;
                if(amount>=min_amount_funds_)
                {

                    auto Inputs_Commitment=c_array(QCryptographicHash::hash(Inputs_Commitments, QCryptographicHash::Blake2b_256));

                    std::vector<std::shared_ptr<qblocks::Output>> the_outputs_{pubOut};
                    auto leftamount=(amount-min_output_pub)/cons_step_index;
                    auto lastamount=(amount-min_output_pub)-leftamount*(cons_step_index-1);
                    for(quint32 i=0;i<cons_step_index;i++)
                    {
                        auto consAddr=std::shared_ptr<Address>(new Ed25519_Address(Account::get_addr({0,1,i}).get_hash()));
                        const auto consUnlcon=std::shared_ptr<qblocks::Unlock_Condition>(new Address_Unlock_Condition(consAddr));
                        const auto consOut= std::shared_ptr<qblocks::Output>
                                (new Basic_Output((i==cons_step_index-1)?
                                                      lastamount
                                                    :leftamount,{consUnlcon},{},{}));
                        the_outputs_.push_back(consOut);
                    }
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
                    Node_Conection::rest_client->send_block(block_);
                    set_publishing(true);
                    qDebug()<<"ffff";
                    respo->deleteLater();
                    qDebug()<<"ffff";
                    set_transfer_funds(0);

                }
                else
                {
                    set_transfer_funds(min_amount_funds_-amount);
                }
                info->deleteLater();
                node_outputs_->deleteLater();
            });
        });
    }

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
