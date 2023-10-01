#pragma once

#include<QObject>
#include<QString>
#include <QtQml/qqmlregistration.h>
#include<booking.hpp>
#include<account.hpp>
#include<nodeConnection.hpp>
#include<set>
#include <queue>


class Book_Server : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QJsonObject  funds READ funds  NOTIFY fundsChanged)
    Q_PROPERTY(QJsonObject  minfunds READ minfunds  NOTIFY minfundsChanged)
    Q_PROPERTY(bool  open READ is_open WRITE setOpen NOTIFY openChanged)
    Q_PROPERTY(bool rpi_server MEMBER m_rpi_server CONSTANT)
    Q_PROPERTY(QString  serverId READ serverId NOTIFY serverIdChanged)
    Q_PROPERTY(ConState  state READ state NOTIFY stateChanged)
    Q_PROPERTY(QJsonArray  payments READ payments NOTIFY paymentsChange)

    QML_ELEMENT
    QML_SINGLETON

public:
    Book_Server(QObject *parent = nullptr);
    enum ConState {
        Publishing = 0,
        Ready
    };
    Q_ENUM(ConState)
    Q_INVOKABLE void restart();

    QString serverId(void)const{return serverId_;}
    void setServerId(QString ser){if(ser!=serverId_){serverId_=ser;emit serverIdChanged();}}
    QJsonObject funds(void)const{return funds_json;}
    QJsonObject minfunds(void)const{return minfunds_json;}
    QByteArray serialize_state(void)const;
    void deserialize_state(const QByteArray &state);
    auto payments(void)const{return payments_;}

#if defined(RPI_SERVER)
    Q_INVOKABLE void open_rpi_box(void);
#endif
    Q_INVOKABLE void try_to_open();
    void setOpen(bool op){if(op!=open){open=op;emit openChanged();}}
    bool is_open()const{return open;}
    ConState state(void)const{return state_;}
    void set_state(ConState state_m){if(state_m!=state_){state_=state_m;emit stateChanged(); }}


signals:
    void fundsChanged();
    void minfundsChanged();
    void openChanged();
    void got_new_booking(QJsonValue nbook);
    void nftAddress(QString);
    void serverIdChanged();
    void notEnought(QJsonObject);
    void accountChanged();
    void finishRestart();
    void stateChanged();
    void paymentsChange();

private:

    void checkFunds(std::vector<qiota::Node_output>  outs);
    void setFunds(quint64 funds_m);
    void setminFunds(quint64 funds_m);
    void init();
    void check_state_output(const std::vector<Node_output> node_output_s);
    void get_restart_state(void);
    void handle_new_book(Node_output node_out);
    void check_nft_to_open(Node_output node_out);
    void handle_init_funds();
    std::shared_ptr<qblocks::Output> get_publish_output(const quint64& amount)const;
    void init_min_amount_funds(void);
    void clean_state(void);


    std::set<Booking> books_;
    bool open,started;
    QObject* reciever;
    ConState state_;
    std::queue<Node_output> queue;
    QJsonObject funds_json,minfunds_json;
    quint64 price_per_hour_;
    QString serverId_;
    QJsonArray payments_;
    quint64 funds_;
    QHash<QString,quint64> total_funds;
    bool m_rpi_server;
};



