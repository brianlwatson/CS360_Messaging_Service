#include "server.h"
#include<sstream>
#include<iostream>
using namespace std;



Server::Server() {
    // setup variables
    buflen_ = 1024;
    sdebugging_flag = 0;
    buf_ = new char[buflen_+1];
}

Server::~Server() {
    delete buf_;
}

void
Server::run() {
    // create and run the server
    create();
    serve();
}

void
Server::create() {
}

void
Server::close_socket() {
}

void
Server::serve() {
    // setup client
    int client;
    struct sockaddr_in client_addr;
    socklen_t clientlen = sizeof(client_addr);

      // accept clients
    while ((client = accept(server_,(struct sockaddr *)&client_addr,&clientlen)) > 0) {

        handle(client);
    }
    close_socket();
}

void
Server::handle(int client) {
    // loop to handle all requests
    while (1) {
        // get a request
        string request = get_request(client);
        // break if client is done or an error occurred
        if (request.empty())
            break;
        // send response

        string response = parse_request(request, client);


        bool success = send_response(client,response);
        // break if an error occurred
        if (not success)
            break;
    }
    close(client);
}

string
Server::parse_request(string buf_, int client) 
{
    string request = "";
 //put this whole block of parsing into handle, right before the response is sent.
        stringstream contents(buf_);

        string request_type;
        contents >> request_type;

        if(sdebugging_flag)
        {
           cout << "Request: " << buf_ << endl;
        }


        if(request_type == "reset")
        {
            request = "OK\n";
            messages.clear();
            return request;
        }

        if(request_type == "put")
        {
            //put(contents.str());
            string name;
            string subject;
            string message;
            string temp = "";
            string length;
            contents >> name;
            contents >> subject;
            contents >> length;

            if(name == "" || subject == "")
            {
                return "error - incorrect put format\n";
            }

            if(!isdigit(length[0]))
            {
                return "error - incorrect put format\n";
            }

            while(contents >> message)
            {
                temp+= " " + message;
            }
            temp.erase(0,1); //erase the first space (laziness)

            int matches = 0; //assign the index
            for(int i = 0; i < messages.size(); i++)
            {
                if(messages[i].getName() == name)
                {
                    matches++;
                }
            }

            int lengthi = atoi(length.c_str());


            if(lengthi > 1023) 
            {
                //cout << "GETLONG: " << get_longrequest(client,lengthi) << endl;
                temp = get_longrequest(client, lengthi);
            }   


            Message m(name,subject, temp, matches + 1);
           // m.toString(); //test to see the contents of the message
            
            int before = messages.size();
            messages.push_back(m);
            
            
            if(sdebugging_flag)
            {
                cout << "put: " << name << " " << subject<< " " << "req: " << request << endl;
            }

            if(before == messages.size() - 1)
            {
                 request = "OK\n";
            }
            else
            {
                request = "error - not added\n";
            }
            return request;
        }

        else if(request_type == "list")
        {
            string name;
            contents >> name;


             if(sdebugging_flag)
            {
                cout << "List request name: " << name << endl;
            }

            if(name == "")
            {
                return "error - no name specified\n";
            }

            stringstream request_ss;

            int matches = 0;
            int found = 0;
            for(int i = 0; i < messages.size(); i++)
            {
                if(messages[i].getName() == name)
                {
                    matches ++;
                    //cout << messages[i].getIndex() << " " << messages[i].getSubject() << endl;
                    request_ss << messages[i].getIndex() << " " << messages[i].getSubject() << "\n";

                }
            }
            stringstream big_out;
            big_out << "list " << matches << "\n";
            big_out << request_ss.rdbuf();
            request = big_out.str();
           
            return request;
        }

        else if(request_type == "get")
        {

            string name;
            contents >> name;

            string desired_index;
            contents >> desired_index;
            int des_index = atoi(desired_index.c_str());

            if(desired_index == "")
            {
                request = "error - No index specified\n";
            }


            for(int i = 0; i < messages.size(); i++)
            {
                if(messages[i].getName() == name && des_index == messages[i].getIndex())
                {
                        stringstream request_ss;
                        request_ss << "message ";
                        request_ss << messages[i].getSubject() << " ";
                        request_ss << messages[i].getMsg().size() << "\n";
                        request_ss << messages[i].getMsg();
                        
                        request = request_ss.str();
                        return request;
                }
                else
                {
                        request =  "error - Message Not Found\n";
                        //break;
                }
            }
            return request;
        }

        else if(request_type == "error")
        {
            return buf_;
        }

     return "error - invalid command\n";
}

string
Server::get_request(int client) {
    string request = "";

    //replace request with a global cache
    //match up with get_request

    // read until we get a newline
    while (request.find("\n") == string::npos) 
    {
        int nread = recv(client,buf_,1024,0);
        if (nread < 0) 
        {
            if (errno == EINTR)
                // the socket call was interrupted -- try again
                continue;
            else
                // an error occurred, so break out
                return "";
        } 

        else if (nread == 0) 
        {
            // the socket is closed
            return "";
        }
        // be sure to use append in case we have binary data
        cache_.append(buf_,nread);// - ???????
        request.append(buf_,nread);
    }

    // a better server would cut off anything after the newline and
    // save it in a cache


    int nlpos = cache_.find("\n");

    // request = cache_.substr(0, nlpos + 1);
    //trim cache_
    cache_.erase(0, nlpos + 1);

    return request;
}

bool
Server::send_response(int client, string response) {
    // prepare to send response

    const char* ptr = response.c_str();
    int nleft = response.length();
    int nwritten;
    // loop to be sure it is all sent
    while (nleft) {
        if ((nwritten = send(client, ptr, nleft, 0)) < 0) {
            if (errno == EINTR) {
                // the socket call was interrupted -- try again
                continue;
            } else {
                // an error occurred, so break out
                perror("write");
                return false;
            }
        } else if (nwritten == 0) {
            // the socket is closed
            return false;
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
        //cout << "COULD BE: " << response << endl;

    return true;
}


void Server::enable_sdebugging()
{
    sdebugging_flag = 1;
}

string
Server::get_longrequest(int client, int length) {
    // read until we get a newline

    while (cache_.size() < length) {//check size
        int nread = recv(client,buf_,1024,0);
        if (nread < 0) {
            if (errno == EINTR)
                // the socket call was interrupted -- try again
                continue;
            else
                // an error occurred, so break out
                return "";
        } else if (nread == 0) {
            // the socket is closed
            return "";
        }
        // be sure to use append in case we have binary data
        cache_.append(buf_,nread);
    }
    // a better server would cut off anything after the newline and
    // save it in a cache
    //change request to cache

    string request = cache_.substr(0, length + 1);
    cache_.erase(0, length + 1);
    cache_ = "";
    return request;
    //return substring of cache
}