# TinyRcon

TinyRcon is a simple, easy to use and powerful GUI application developed using modern C++ language features, 
the Win32 API and the standalone version of the ASIO C++ networking library. The project consists of two major parts:

 1. TinyRcon client which runs on a user's (administrator) computer. 
 2. TinyRcon server which can be hosted on the same user's computer as well, although that's not a requirement at all. 

TinyRcon clients communicate with the The TinyRcon server by exchanging short messages with it using the simple 
UDP transport protocol. All ban entries of various types are automatically updated and synchronized among all logged
in authorized TinyRcon client users. There are three different methods implemented in both TinyRcon client and TinyRcon
server that work together to accomplish the task of automatic synchronization and sharing of all registered active ban 
entries for every logged in authorized administrator.
The project is currently under active development and it is not a cross-platform GUI application at the moment.
I plan to to make it cross-platform shortly.
