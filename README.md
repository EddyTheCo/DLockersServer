# [DLockers  Server](https://eddytheco.github.io/DLockersServer/wasm/)


## Proof of Concept on implementing decentralized applications on the IOTA network.

This application can be seen as a decentralized server that allows you to book a locker by paying with Shimmer.
The application is set to use the Shimmer Testnet and custom libraries developed by me.
In order to use the application one needs to set the address of the node to connect.
The Proof of Work has to be performed by the node (by setting the JWT for protected routes, by enabling PoW in the node...).
In principle it will also work for the shimmer mainnet by setting the node to a mainnet one(I have not tried).
This application is meant to be used on the testnet.
If using the mainnet **you are the ONLY responsible for the eventual loss of your funds**.

## How to use it

In order to book a box one can use the [client](https://eddytheco.github.io/DLockersClient/wasm/).
The communication between server and client relies on creating outputs on the ledger.
Because of that, the server needs an initial amount of funds to be able to publish its state on the ledger. 
These initial funds are always own by the server. This proof of concept uses a random seed for the address
creation(if reload the page the server loses its funds).


From now, one can connect different clients to the same server but not different server to the same client.
This can be easily implemented and will allow you to choose for example the server closest to you.


The server looks for new bookings sent by the clients and checks the validity of these bookings.
A booking is valid if it has an allowed  start and finish time, a code and the price payed for the booking is correct.
If everything is fine the server collect the money from the client and update its internal state.


The server allows you to open the locker if it has a booking at that time and you entered a correct pin.
For the propose of this proof of concept  the  pin is a number of 5 digits but in real applications the pin should be a
large string.

## The bigger picture

* Any person can start renting physical space for lockers.
* You do not depend on centralize company servers to store your business data or to process payments.
* As longer as you maintain a node(blockchain), you gainings  and business data are cryptographically secured. 
* The code is open source and grant you certain rights and responsibilities to respect other people rights.


