# The Come Back
## HTTP Server 
Cal Blanco
05/12/2022



>Okay so I really have slacked on  the readme but im gonna try and do some stuff rn  - me a while ago

## UPDATE for ASGN3:
### Another crazy asgn
Yea this one was a bit of doozie. Gonna just submit what i have for now and hope that some stuff still works. Get back to it tomorrow

I added a decent amount. I worked on splitting up my code into some external .h files which has totally helped me with my overall readability

**httpeen.h** : has some methods to handle http methods
**farser.h** : has methods to parse the input strings
**becool.h** : defines a bounded queue that is used for tasks (the work queue)

Gonna try and get a lot of stuff done in this week while working on asng 4

Currently I think a lof of my issues are coming in the form of memory (stack vars / heap vars) and the visibility the  thread has
I want to finish up the task_t update i was doing but I'm gonna treat myself and get sleep tongight plus save my last grace day for asgn4.

Currently in the process 

## Qualitative: 
### First and foremost this has been crazy hard.
 I don't think I've ever spent so much time coding before, or reading through manuals. I seriously love it not in a complaining way. It has just been really hard!

Assignment 1 I started way to late and really struggled to understand the whole process. 

I ended up turning in a subpar program just so that I would not end up using too many grace days on it. But that meant I needed to do a lot of changing to get it to something I could build the next assignments on top of.

I think made a lot of bad assumptions out of the gate with asgn1
- I thought we would get the whole msg in one go
- Then I thought we would either get the msg or not but not a mix

I put in the hours but I finally have something that is passing tests in asgn1 and all in asng2! Im super proud of myself honestly! :) This class is awesome 


## Big Changes

Assignment 1 I put together with duct tape basically. My handle_connection function was a mess of random conditionals with basically no understanding or forethought. I was just trying to pass some tests and get on the board. It was also like **200+** lines of code. I had a function for get but did the puts inside for some reason. I really don't know I was just an absolute mess.

I had to fully think about how the message was coming in and how to interperet that message in certain scenarios.

I broke my code down into different chunks 
0. Overall Flow
1. Getting the Request Line information
2. Getting Header information / Validating Headers
3. Opening the file with correct permissions
4. Executing the appropriate method on said file
5. Generating a Response 
6. Logging what ever we did

7. Moving forward

> obv 5 was added in to meet asgn2 criteria so not technicall part of asgn 1 rewrite

> Also one interesting component has been understanding when to respond and log, and how to get that flow correctly placed in the system


## 0. Overall Flow  : Handle_Connection(int connfd)

**Vars:**
- `char *buff : stores read data`
- `size_t br : stores # of bytes read from reads(3)`
- `char URI[20] : stores URI`
- `int method : stores an int representation for the method we want to execute`
- `int conlen : content length`
- `int RID : request id `

**Flow:**
1. Recieve a message from the client

Look for key indicators. I assume that there must atleast be a request in the read. If there is not a CRLF in `buff` after the first read then i return a `status code 400`.

2. Attempt to parse the request line from the buffer `buff`
using `requestInfo(3)`. This function will generate information for `method` and `URI`

3. Is there a double CRLF (end of header in the buffer?)
If yes : call `headers(3)` with current buffer, else call `read()` again and then call `headers(3)` on that buffer. This function will generate information for `conlen` and `RID`

4. We should have enough information to execute the request if we reach this stage. `method` = 1 perform `get(3)`, `method` =2 perform `put(5)`, and `method`=3 performs `append(5)`

5. Each method handles writing a successful response and only error responses are written by `handle_connection`


## 1. Getting Request Line Information

Getting the request line information `method` and `URI` is done by the function `requestInfo(3)`

### int requestInfo(char *s, int *method, char *uri)
*Input Vars*
- `char *s` : cstring (should be the buffer from `handle_connection`)
- `int *method`: pointer to the method var in `handle_connection` to set
- `char *uri`: string passed by `handle_connection` to be set

*Operation*
- `requestInfo(3)` scans `s` until it finds `\r\n`. It then copies the contents of `s[0..pos(\r\n)]` into a temp cstring to split up with `sscanf`. 

- After the string is tokenized each portion is checked for its given validity. Method is alphabetical, URI alphanumeric with the .- included. Version is also validated at this step. If any of these checks fail a -1 is returned to `handle_connection`

- `method` is set by `strnmcp(3)` on the string reported by the sscanf

- if nothing bad happens function returns a 1 for success.



## 2. Getting Header Information

Getting header information (`conlen` and `RID`) is done with `header(3)`

### int headers(char *s, int *cl, int *rid)
*Input Vars*
- `char *s` : cstring (should be a buffer from a read)
- `int *cl` : pointer to conlen variable 
- `int *rid` : pointer to RID variable

*Operation*
- headers(3) is very similar to requestInfo(3) in that it takes the input buffer `s` and tries to find a CRLF and the empty header CRLF.

- After it finds these positions it then makes a duplicate of the buffer of size = `2xCRLF - CRLF`. Which is then tokenized by `CRLF` to find each header line. 

- The header lines themselves are individually validated for ascii keys

- in this step `strstr` is also used to check for the headers `Content-Length:` and `Request-Id` 

- if nothing bad happens a 1 is returned to `handle_connection` letting it know to continue 


## 3/4 Getting file perms and Operating on file

This is done by three main functions
- `get(3)`
- `put(5)`
- `append(5)`

An auxilary function `writeToURI(3)` is used to help `put` and `append` write to the `uri`

## GET
### int get(int connfd, char *uri, int uid)
*Input Vars*
- `int connfd`: connection file desc
- `char *uri` : uri to try and open
- `int uid` : request id (unique id is why i named it uid)

*Operation*
 - Tries to open the file at `uri` with `O_RDONLY` mode

 - read all the bytes from the file and write them to `connfd`

 - write the `ok` status response after if successful and return a 1 to let `handle_connection` know we did our job

 - Anything else returned is an error code and `handle_connection` will deal with it

## PUT
### int put(int connfd, char *uri, int cl, int uid, int more, char *b)
*Input Vars*
- `int connfd`: connection file desc
- `char *uri` : uri to try and open
- `int uid` : request id (unique id is why i named it uid)
- `int cl` : content length
- `int more`: # of bytes in current buffer that still need to write
- `char *b` a pointer to where those bytes should come from

*Operation*
- File flags are `O_WRONLY` and `O_TRUNC`

- Check to see if the file exists. If not create it adding `O_CREAT` to the flags

- if `more` is greater than 0 we should have bytes in `b` that need to be written to `uri`

- We then write `cl` - `more` bytes to the uri in following reads by calling `writeToURI(3)`

- write the `ok` status response after if successful and return a 1 to let `handle_connection` know we did our job

- Anything else returned is an error code and `handle_connection` will deal with it

## APPEND
### int append(int connfd, char *uri, int cl, int uid, int more, char *b)
*Input Vars*
- `int connfd`: connection file desc
- `char *uri` : uri to try and open
- `int uid` : request id (unique id is why i named it uid)
- `int cl` : content length
- `int more`: # of bytes in current buffer that still need to write
- `char *b` a pointer to where those bytes should come from

*Operation*
- File flags are `O_WRONLY` and `O_APPEND`

- if `more` is greater than 0 we should have bytes in `b` that need to be written to `uri`

- We then write `cl` - `more` bytes to the uri in following reads by calling `writeToURI(3)`

 - write the `ok` status response after if successful and return a 1 to let `handle_connection` know we did our job

 - Anything else returned is an error code and `handle_connection` will deal with it

## WRITE TO URI
### int writeToURI(int connfd, int fd, int cl)
*Input Vars*
- `int connfd` : connection file desc
- `int fd` : the filedesc of the uri we are writing to
- `int cl` : content length remaining to write

*Operation*
- Attempt to read more bytes from `connfd`. Then write `cl` bytes to `fd`


## 5. Generating A Response

This program generates responses a few different ways. 

For error responses the function `genResponse(2)` is used. 

## GEN RESPONSE
### void genResponse(int code, int connfd)
*Input Vars*
- `int code` : status code
- `int connfd` :connfd

*Operation*
- use a switch case on `status code` to write an appropriate response to the client. 

I wanted to use this function more but I ended up handling opening the file / checking for existence inside the PUT function, and GET can't use a preset one so I ended up making the methods genrerate their own response upon their sucess. Only faiures are reported with genResponse(3)


## 6. Logging it all

I used a function called `lerg(4)` which just takes in the expected arguments for the LOG macro and puts them in the right order. 

This function gets called in `handle_connection` when an error is reported by `get` `put` or `append`. 

If the method is successful it will `lerg` the information at the same time it writes it success response out 


## 7. Moving Forward
Theres still a lot I want to iron out with my code, and some more modularity I want to employ, but I'm really happy that I was able to pass the tests I am with the code I have. Did not think I'd make it this far!

I want to try and smooth up the genResponse function. I also think there could be some value in this side venture I tried of generating the opening flags with the `int method` and then passing a filedesc to the methods so they can operate on them.

I have a lot of ideas but I think I can finally breathe after feeling weeks behind on asgn1 and asgn2 




