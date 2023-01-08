# [DLockers  Server](https://eddytheco.github.io/DLockersServer/wasm/)


## Proof of Concept on implementing decentralized applications on the [IOTA](https://www.iota.org/) network.

This application can be seen as a decentralized server that allows you to book a locker by paying with Shimmer.
The application is set to use the [Shimmer Testnet](https://explorer.shimmer.network/testnet/)
and custom libraries developed by me.
For sending a block to the network the application needs to perform Proof of Work.
My implementation of proof of work it is not optimized and this will take much time on the browser, please be patient 
or do a pull request to this [repo](https://github.com/EddyTheCo/Qpow-IOTA) with a faster implementation.


In order to book a box one can use the [client](https://eddytheco.github.io/DLockersClient/wasm/).
The communication between server and client relies on creating outputs on the ledger.
Because of that, the server needs an initial amount of funds to be able to publish its state on the ledger. 
These initial funds are always own by the server. This proof of concept uses a random seed for the address
creation(if reload the page the server loses its funds).
* *For transferring funds to the server one can use a wallet like [firefly](https://firefly.iota.org/).*


From now, one can connect different clients to the same server but not different server to the same client.
This can be easily implemented and will allow you to choose for example the server closest to you.


The server looks for new bookings sent by the clients and checks the validity of these bookings.
A booking is valid if it has an allowed  start and finish time, a code and the price payed for the booking is correct.
If everything is fine the server collect the money from the client and update its internal state.


The server allows you to open the locker if it has a booking at that time and you entered a correct pin.
For the propose of this proof of concept  the  pin is a number of 5 digits but in real applications the pin should be a
large string.

**This has only be tested on the Firefox browser**
