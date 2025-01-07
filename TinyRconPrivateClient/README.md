# TinyRcon

TinyRcon is a relatively simplistic, easy to use GUI application developed using modern C++ language features, 
the Win32 API and the standalone version of the ASIO C++ networking library. The project consists of two major parts: 
 1. TinyRcon client which runs on a user's (administrator) computer. 
 2. TinyRcon server which can be hosted on the same user's computer as well, although that's not a requirement at all. 
TinyRcon clients communicate with the The TinyRcon server by exchanging short messages with it using the simple 
UDP transport protocol. TinyRcon clients are able to keep their ban entries updated and synchronized with each other 
TinyRcon client user by logging in to the TinyRcon server. The project is currently under active development and 
it is not a cross-platform GUI application at the moment. I plan to to make it cross-platform shortly.