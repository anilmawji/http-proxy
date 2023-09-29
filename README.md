# http-proxy
Web proxy that blocks webpages containing select keywords. Made to work on a Linux environment.

Compile with `gcc proxy.c -o proxy` and run with `./proxy`. See the file for necessary information about ports.

While the script is running, use `BLOCK <keyword>` to add a keyword to the block list, and `UNBLOCK` to unblock all keywords. 
