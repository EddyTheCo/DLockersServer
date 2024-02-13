# [DLockers  Server](https://eddytheco.github.io/DLockersServer/wasm/)

This repo is now archived, please refer to [Dlockers repo](https://github.com/EddyTheCo/DLockers)

## Proof of Concept on implementing decentralized applications on the IOTA network.

This application can be seen as a decentralized server that allows you to book a locker by paying with cryptos.
In order to use the application one needs to set the address of the node to connect.
The Proof of Work has to be performed by the node (by setting the JWT for protected routes, by enabling PoW in the node...).
In principle it will also work for the mainnet by setting the node to a mainnet one(I have not tried).
This application is meant to be used on the testnet.
If using the mainnet **you are the ONLY responsible for the eventual loss of your funds**.

## How to use it

In order to book a box one can use the [client](https://eddytheco.github.io/DLockersClient/wasm/).
The communication between server and client relies on creating outputs on the ledger.
Because of that, the server needs an initial amount of funds to be able to publish its state on the ledger. 
These initial funds are always own by the server. This proof of concept uses a random seed for the address
creation(if reload the page the server loses its funds).


From now, one can connect different clients to the same server but not different server to the same client.
This can be easily implemented and will allow you to choose for example, the server closest to you.


The server looks for new bookings sent by the clients and checks the validity of these bookings.
A booking is valid if it has an allowed  start and finish time and the price payed for the booking is correct.
If everything is fine the server collect the money from the client, update its internal state and send a NFT to the client.


The server allows you to open the locker if it has a booking at that time and you own a NFT signed by the server.
The signed NFT has immutable metadata that reference certain time interval of the bookings the client has paid.

## The bigger picture

* Any person can start renting physical space for lockers.
* You do not depend on centralize company servers to store your business data or to process payments.
* As longer as you maintain a node(blockchain), your gainings  and business data are cryptographically secured. 
* The code is open source and grant you certain rights and responsibilities to respect other people rights.
* By using NFTs the client can pass the right to open a box to other client.

### CORS header 'Access-Control-Allow-Origin' missing

When using the browser application and your node, the API request could be denied with the return 'Reason: CORS header 'Access-Control-Allow-Origin' missing'.
In that case, one needs to set the Access-Control-Allow-Origin header's value as explained [here](https://developer.mozilla.org/en-US/docs/Web/HTTP/CORS/Errors/CORSMissingAllowOrigin).

If you use the docker setup of Hornet just add 

```
- "traefik.http.middlewares.cors.headers.customResponseHeaders.Access-Control-Allow-Origin=https://eddytheco.github.io"
- "traefik.http.routers.hornet.middlewares=cors"
```
to docker-compose.yml in the traefik section. Such that browser API requests from https://eddytheco.github.io are accepted  by your node.
