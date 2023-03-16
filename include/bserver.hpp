#pragma once

#include<QObject>
#include<QString>
#include <QtQml/qqmlregistration.h>
#include<booking.hpp>
#include<account.hpp>
#include"nodeConnection.hpp"
#include<set>
#include <queue>


class Book_Server : public QObject
{
    Q_OBJECT

    Q_PROPERTY(unsigned int  price_per_hour READ get_price_per_hour WRITE set_price_per_hour NOTIFY price_per_hour_changed)
    Q_PROPERTY(unsigned int  transfer_funds READ transfer_funds NOTIFY transfer_funds_changed)
    Q_PROPERTY(bool  open READ is_open NOTIFY open_changed)
    Q_PROPERTY(ConState  state READ state NOTIFY stateChanged)

    QML_ELEMENT
    QML_SINGLETON

public:
    Book_Server();
    enum ConState {
        Publishing = 0,
        Ready
    };
    Q_ENUM(ConState)
    Q_INVOKABLE void restart(){
        init_min_amount_funds();
    }

    quint64 get_price_per_hour(void)const{return price_per_hour_;}
    void set_price_per_hour(quint64 pph){price_per_hour_=pph;emit price_per_hour_changed();}



    quint64 transfer_funds()const{return transfer_funds_;}

    Q_INVOKABLE bool try_to_open(QString code);
    void close_it(void){open=false;emit open_changed(open);}
    bool is_open()const{return open;}
    ConState state(void)const{return state_;}
    void set_state(ConState state_m){if(state_m!=state_){state_=state_m;emit stateChanged(); }}


signals:
    void price_per_hour_changed(void);
    void open_changed(bool);
    void got_new_booking(const Booking nbook);

    void transfer_funds_changed(void);
    void accountChanged();
    void finishRestart();
    void stateChanged();

private:
    void init();
    void check_state_output(const std::vector<Node_output> node_output_s);
    void get_restart_state(void);
    void wait_new_state(void);
    void handle_new_book(Node_output node_out);
    void handle_init_funds();
    std::shared_ptr<qblocks::Output> get_publish_output(const quint64& amount)const;
    void set_transfer_funds(quint64 traFund)
    {if(transfer_funds_!=traFund){transfer_funds_=traFund;emit transfer_funds_changed();}}
    void init_min_amount_funds(void);
    void clean_state(void);
    void publish_state();
    quint64 price_per_hour_;
    quint64 transfer_funds_;
    void collect_output(const Node_output &output);
    std::set<Booking> books_;
    bool open,started;
    QObject* reciever;
    ConState state_;
    std::queue<Node_output> queue;
};



