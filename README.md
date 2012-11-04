LIXY
====

lixy (LISP on Linux for Vyatta) is Linux uarland Locator/ID Separation
Protocol implementation for vyatta. it is for trial use.

lixy requires uthash-dev.

	 % git clone git://github.com/upa/lixy.git
	 % cd lixy
	 % make install-vyatta

and you can configure lisp through vyatta.


XTR configuration example


	 protocols {
	     lisp {
	         locator 192.168.0.1 {
	             priority 10
	         }
	         map-server 172.16.0.1
	         site hoge {
	             authentication-key ****************
	             prefix 10.10.0.0/24
	         }
	     }
	 }



PXTR configuration example


	 protocols {
	     lisp {
	         locator 192.168.3.1 {
	         }
	         map-server 172.16.0.1
	     }
	 }


if you use lixy without vyatta, you can use _lixyctl_ command
to configure lixy.


lixy is a minimal implementation for lisp xtr.
So it does not support many features e.g. security bit, echo nonce...


ToDo
----
- vyatta show commands
- packet counter
- tunnel interface for pxtr
- kernel module data plane
