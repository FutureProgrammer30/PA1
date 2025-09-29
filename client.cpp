/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Renee Gunukula
	UIN: 234002407
	Date: 09/18/25
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <fstream>
#include <iostream>
#include <cmath>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>


using namespace std;




int main (int argc, char *argv[]) {
    int opt;
    int p = 1;
    double t = 0.004;
    int e = 1;
    int m = MAX_MESSAGE;
    bool new_chan = false;
    vector<FIFORequestChannel*> channels;

	bool have_t = false; 
	bool have_filename = false; 


   
    string filename = "";
    while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
        switch (opt) {
            case 'p':
                p = atoi (optarg);
                break;
            case 't':
                t = atof (optarg);
				have_t = true; 
                break;
            case 'e':
                e = atoi (optarg);
                break;
            case 'f':
                filename = optarg;
				have_filename = true; 
                break;
            case 'm':
                m = atoi(optarg);
                break;
            case 'c':
                new_chan = true;
                break;
        }
    }


    // Give arguments for the server
    // Server needs './server', '-m', '<val for -m arg>', 'NULL'
    // -m: maximum number of bytes that a server can send to the client and vice versa
    // fork
    // In the child, run execvp using the server arguments.
    // start a fork and check if pid is a child, if it is start a server using execvp. Inside execvp,
    // the path is the path to the server


    // Task 1. Create server, with correct format as requested
    pid_t pid = fork();        
    if (pid == -1) {                        
        perror("fork");
        return 1;
    }

    // std::string mstr = std::to_string(m);
    // char* args[4];
    // args [0] = (char*) "./server";
    // args [1] = (char*) "-m";
    // args [2] = (char*)mstr.c_str();
    // args [3] = NULL;


    if (pid == 0) {                        
        std::string mstr = std::to_string(m);
        char* srv_argv[] = {
            (char*)"./server",             
            (char*)"-m",                    
            (char*)mstr.c_str(),         
            nullptr                          
        };
        execvp(srv_argv[0], srv_argv);       
        perror("execvp ./server");          
        _exit(-1);                     
        // if(execvp(args[0], args) == -1) {
        //  perror("exec");
        //  return EXIT_FAILURE;
        // }
    }
   


    FIFORequestChannel cont_chan("control", FIFORequestChannel::CLIENT_SIDE);
    channels.push_back(&cont_chan);
    //Single datapoint, only run p,t,e != -1
    if(new_chan) {
        //send newchannel request to the server
        MESSAGE_TYPE nc = NEWCHANNEL_MSG;
        cont_chan.cwrite(&nc, sizeof(MESSAGE_TYPE));
        //create a variable to hold the name
        // cread the response from the server
        // call the FIFORequestChannel constructor with the name from the server
        //dynamically create the channel, call new with constructor so that you can use it outside of the if statement
        //Structure to hold channels, and then we you're calling these requests, you just need to call the correct channel
        //push the new channel into the vector
        char cname[MAX_MESSAGE];
        memset(cname, 0, sizeof(cname));
        int n = cont_chan.cread(cname, sizeof(cname) - 1); 
        if (n <= 0) {
            cerr << "ERROR: failed to receive new channel name\n";
            return 1;
        }
        cname[n] = '\0';                  
        FIFORequestChannel* data_chan = new FIFORequestChannel(std::string(cname), FIFORequestChannel::CLIENT_SIDE);                                // 4) store it so we can use the newest channel via channels.back() rm
        channels.push_back(data_chan);


    }


    //use last channel in our vector to send all of our requests
    FIFORequestChannel chan = *(channels.back());
    //Make connection first and obtain data_channel if needed (based on flag above)
   
    // example data point request
    char buf[MAX_MESSAGE]; // 256
    datamsg x(p, t, e); //change from hardcoding to user's values


    // Task 2. Send datamsg request:
    //      Case 1) Requesting data for a specific person's ecg data with timestamp
    //      Case 2) Requesting data with 1000 points from specific patient to recieved/x1
   
    memcpy(buf, &x, sizeof(datamsg));
    chan.cwrite(buf, sizeof(datamsg)); // question, write it to the server datamsg through our control channel
    double reply;
    chan.cread(&reply, sizeof(double)); //answer
    cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;


   
   
    //Else, if p != 1, request 1000 datapoints
    //Loop over 1st 1000 lines
    //Send request for ecg 1
    //Send request for ecg 2
    // Write line to recieved/x1.csv


    if(!have_t) {
        std::ofstream outdata;
        outdata.open("received/x1.csv");


        if(!outdata) {
            cerr << "Error: file could not be opened, boo" << endl;
            return 1;
        }


        // if(filename == "") {
        //  cerr << "filename not given" <<endl;
        //  return 1;
        // }
       
        for(int i = 0; i < 1000; i++) {




            double current_seconds = i * 0.004;
            datamsg d(p, current_seconds, 1);


            memcpy(buf, &d, sizeof(d));
            chan.cwrite(buf, sizeof(d)); // question, write it to the server datamsg through our control channel
            double first_ecg = 0;
            chan.cread(&first_ecg, sizeof(double)); //answer


            datamsg d2(p, current_seconds, 2);


            memcpy(buf, &d2, sizeof(d2));
            chan.cwrite(buf, sizeof(d2)); // question, write it to the server datamsg through our control channel
            double second_ecg = 0;
            chan.cread(&second_ecg, sizeof(double)); //answer


            outdata << current_seconds << ',' << first_ecg << ',' << second_ecg << '\n';


           


        }
        outdata.close();


    }


    // Task 3. Send filemsg request:
    //      Case 3) Requesting data from file (remember chunking)
    //              recall format is <filemsg class> + <name of file>
    //              fopen to get size of file, file size is filename.size() + 1
    // sending a non-sense message, you need to change this
	if(have_filename) {
		filemsg fm(0, 0); //Asking the server how big the file is
		string fname = filename;
	
		int len = sizeof(filemsg) + (fname.size() + 1);
		char* buf2 = new char[len]; //len is len of filemsg + len of filename + 1 (null terminator)
		memcpy(buf2, &fm, sizeof(filemsg)); // copying file message into buffer and copy file name into the buffer
		strcpy(buf2 + sizeof(filemsg), fname.c_str());
		chan.cwrite(buf2, len);  // I want the file length; and then we send our buffer to the server


		int64_t filesize = 0;
		chan.cread(&filesize, sizeof(int64_t));


		char* buf3 = new char[m];//create buffer of size buff capacity (m)
		// Loop over the segments in the file filesize / buff capacity[-m]
		// create filemsg instance


		std::string outpath = "received/" + filename;
		std::ofstream out(outpath, std::ios::binary);
		if(!out) {
			cerr << "Error: file could not be opened, yo" << endl;
			exit(1);
		}
	
		filemsg* file_req = (filemsg*)buf2; //charstar to filemsg. allows us to reuse buf2
		//while requesting file requests
		int64_t offset = 0;
		//double result = ceil(filesize/m);


		int result = (int)ceil((double)filesize / m);
		for(int i = 1; i <= result; i++) {
			if(i != result) {
				file_req->offset = offset;
				file_req->length = m;
				chan.cwrite(buf2, len);
				chan.cread(buf3, m);
				out.write(buf3, m);
				offset += m;
			
			} else {
				int64_t new_one = filesize - offset;
				file_req->offset = offset;
				file_req->length = (int)new_one;
				chan.cwrite(buf2, len);
				chan.cread(buf3, new_one);
				out.write(buf3, new_one);
			}
		}

		out.close();




		delete[] buf2;
		delete[] buf3;
	}
    //If creating the channEL DYNAMICALLY, DELETE IT
    //If necessary, close and delete the new channel
    MESSAGE_TYPE a = QUIT_MSG;
    if(new_chan) {
        FIFORequestChannel* data_chan = channels.back();
        //do your close and delete
        data_chan->cwrite(&a, sizeof(MESSAGE_TYPE));
        channels.pop_back();
        delete data_chan;
        data_chan = nullptr;
       
    }


    //Close connection to DataChannel if needed
   
    // closing the channel    
    MESSAGE_TYPE b = QUIT_MSG;
    cont_chan.cwrite(&b, sizeof(MESSAGE_TYPE));
    //chan.cwrite(&m, sizeof(MESSAGE_TYPE));


}
