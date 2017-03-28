service telnet
{
	disable 	= no
	flags		= REUSE
	socket_type	= stream        
	wait		= no
	user		= root
	server		= /bin/telnetd
	log_on_failure	+= USERID
	only_from	= localhost $NET_REMOTE_ACCESS_FROM
}
