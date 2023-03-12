#pragma once

#include<QObject>
#include<QString>
#include <QtQml/qqmlregistration.h>
#include<booking.hpp>
#include<account.hpp>
#include"nodeConnection.hpp"
#include<set>


class Book_Server : public QObject
{
    Q_OBJECT

    Q_PROPERTY(unsigned int  price_per_hour READ get_price_per_hour WRITE set_price_per_hour NOTIFY price_per_hour_changed)
    Q_PROPERTY(unsigned int  transfer_funds READ transfer_funds NOTIFY transfer_funds_changed)
    Q_PROPERTY(bool  open READ is_open NOTIFY open_changed)
    Q_PROPERTY(bool  publishing READ publishing NOTIFY publishing_changed)


    QML_ELEMENT
    QML_SINGLETON

public:
    Book_Server();
    Q_INVOKABLE void restart(){
        init_min_amount_funds();
    }

    quint64 get_price_per_hour(void)const{return price_per_hour_;}
    void set_price_per_hour(quint64 pph){price_per_hour_=pph;emit price_per_hour_changed();}

    void set_publishing(bool pub){publishing_=pub;emit publishing_changed();};
    bool publishing(void)const{return publishing_;}

    quint64 transfer_funds()const{return transfer_funds_;}

    Q_INVOKABLE bool try_to_open(QString code);
    void close_it(void){open=false;emit open_changed(open);}
    bool is_open()const{return open;}




signals:
    void price_per_hour_changed(void);
    void open_changed(bool);
    void got_new_booking(const Booking nbook);
    void publishing_changed(void);
    void transfer_funds_changed(void);
    void accountChanged();

private:
    void cleanConsolidation() const;
    void handle_new_book(Node_output node_out);
    void handle_init_funds(ResponseMqtt *respo);
    std::shared_ptr<qblocks::Output> get_publish_output(const quint64& amount)const;
    void set_transfer_funds(quint64 traFund)
    {if(transfer_funds_!=traFund){transfer_funds_=traFund;emit transfer_funds_changed();}}
    void init_min_amount_funds(void);
    Booking get_new_booking_from_metadata(qblocks::fl_array<quint16> metadata)const;
    qblocks::fl_array<quint16> get_state_metadata(void)const;
    void clean_state(void);
    void publish_state();
    quint64 price_per_hour_;
    quint64 min_amount_funds_,transfer_funds_;
    quint32 consolidation_index;
    quint32 cons_step_index;
    void collect_output(const Node_output &output);
    std::set<Booking> books_;
    bool open,publishing_;
    QObject* reciever;

};



