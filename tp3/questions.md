For each of the questions below we used the following code to process the data
from the tables :

```js
string = `1     165095720   521     12
2   165842766   521     12
3   165458792   514     12
4   163235772   512     12`

// Parse data from table
data = string.split('\n').map(a => a.split(' \t').map(b => Number(b))).map(a => {
    return {connection: a[0], bytes: a[1], duration: a[2], rtt: a[3]}
})

function updateData(connection) {
    connection.throughputBits = 8 * connection.bytes / connection.duration;
    connection.lossRate = (1.22 * 1500 * 8 / (connection.rtt * connection.throughputBits)) ** 2;
    return connection;
}

// sum throughputs (bits)
total_bandwidth = data.map(updateData).reduce((sum, connection) => sum + connection.throughputBits, 0) 

header = `Con \tBytes \tTime \tRTT \tThroughput (bits) \tRelative performance\n` 

// If the network is fair, this product should be the same for every connection
con1_perf = data[0].throughputBits * data[0].rtt 

// Output table with details for each connection
header + data.map(updateData).map(a => `${a.connection} \t${a.bytes} \t${a.duration} \t${a.rtt} \t${Math.floor(a.throughputBits)} \t(${Math.floor(100*a.throughputBits / total_bandwidth)} %) \t${Math.floor(a.throughputBits * a.rtt * 100 / con1_perf)}`).join('\n')
```

We measure relative performance by calculating the throughput * RTT product
(higher is better) and compare it to that of the first connection. If the
network is fair, this result should be roughly the same for each connection.


16. What is the throughput of each of the connections in bps (bits per second)?
    What is the total bandwidth of the host on which the clients are running?
    Discuss the TCP fairness for this case.

Con	Bytes 	    Time 	RTT	Throughput (bits) 	Relative performance
1 	165095720 	521 	12 	2535059 	(24 %) 	100
2 	165842766 	521 	12 	2546529 	(24 %) 	100
3 	165458792 	514 	12 	2575234 	(25 %) 	101
4 	163235772 	512 	12 	2550558 	(24 %) 	100

Total thoughput : 10207382 bits/s

Each connection uses about a fourth of the total available bandwidth so we can
say the situation is fair as RTTs are identical.

Characteristics are nearly identical for all connections.

17. What is the throughput of each of the connections in bps (bits per second)?
    What is the total bandwidth of the host on which the clients are running?
    Discuss the TCP fairness for this case.

Con	Bytes 	    Time 	RTT 	Throughput (bits) 	Relative performance
1 	261319130 	90 	    13 	    23228367 	(24 %) 	100
2 	175995832 	90 	    35 	    15644073 	(16 %) 	181
3 	151894552 	90 	    68 	    13501737 	(14 %) 	304
4 	140388568 	90 	    73 	    12478983 	(13 %) 	301
5 	108610702 	90 	    49 	    9654284 	(10 %) 	156
6 	70644690 	90 	    33 	    6279528 	(6 %) 	68
7 	65744938 	90 	    135 	5843994 	(6 %) 	261
8 	43212876 	90 	    326 	3841144 	(4 %) 	414
9 	39222524 	90 	    322 	3486446 	(3 %) 	371

Total throughput : 93958561 bits/s

As we can see, the relative bandwidth usage varies considerable between
connections (3 to 24 % !) but so does RTT. If we take the first connection for
reference, relative performance is not homogeneous. If the network were fair,
the throughput * RTT product would be roughly equal for each connection ! This
is not the case, therefore it is unfair.

Consider connections 6 and 8. Connection 6 has ten times better RTT but only
double the bandwidth ! If it were fair, ten times better RTT would translate to
ten times better bandwidth.

Now consider connections 2 and 6. They have similar RTT but 2,5 times
throughput difference ! If it were fair we would observe similar throughput.

Connections 3 and 4 are fair. Connections 2 and 5 are nearly fair
(performance-wise).

All connections are observed for the same amount of time but do not have the
same data throughput or RTT.

18. Discuss the TCP fairness for this case. How does it differ from the
    previous cases, and how is it affected by the use of BitTorrent?

Con	Bytes 	    Time 	RTT 	Throughput (bits) 	Relative performance
1 	108851134 	58 	    40 	    15013949 	(18 %) 	100
2 	90435681 	58 	    36 	    12473887 	(15 %) 	74
3 	57971584 	53 	    100 	8750427 	(10 %) 	145
4 	32000012 	29 	    68 	    8827589 	(10 %) 	99
5 	32557334 	35 	    31 	    7441676 	(9 %) 	38
6 	27199361 	31 	    33 	    7019189 	(8 %) 	38
7 	26329578 	31 	    122 	6794729 	(8 %) 	138
8 	38834490 	56 	    146 	5547784 	(6 %) 	134
9 	23571761 	35 	    74 	    5387831 	(6 %) 	66
10 	36252962 	55 	    66 	    5273158 	(6 %) 	57

Total throughput : 93958561 bit/s

With BitTorrent, each peer has a different internet connection, and cannot
allocate the same amount of bandwidth, even if the connecting peer is close
(with a low RTT). For example, connection 5 and 6 have the best RTT but the
worst relative performance.
