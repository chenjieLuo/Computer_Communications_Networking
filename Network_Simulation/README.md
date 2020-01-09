This folder is created for Network Simulation assignment in ECEN 602. 

In this simulation assignment, Chenjie Luo(UIN: 324007289) is mainly responsible for the simulation and output files of test cases while Yu-Wen Chen(UIN: 227009499) is mainly responsible for table of the simulation results and the report. 

For the code, it is mainly divided into three parts: simulation time flow, initialization and function definitions. For the time flow, simulation started at time = 0 sec. So at time = 0 sec we started ftp1 and ftp2 and initialized output file handlers. Since instructions said we didn't need to collect data first 100 sec, we started put calculated throughputs to output files from time = 100 sec. At t = 400 sec, we finished the simulation and closed the file handlers. 

For the architecture, firstly users will input two parameters named: TCP version as well as case number. According to these two parameters, we defined connection and bandwidth between sources and routers as well as creating tcp sending agents under matched version. Next we will set up and connect TCPsinks with receivers. TCPsinks are generally used as receivers to keep track number of bytes received. 

For errata, I set the acceptance of the two input parameters. For TCP version only "VEGAS" or "SACK" will be accepted while for case number only 1,2,3 will be accepted. All rest inputs are classified as illegal inputs and error message will be printed on the terminal. The simulation will then exit. 
