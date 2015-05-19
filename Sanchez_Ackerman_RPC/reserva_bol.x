/*		reserva_bol.x			*/

struct seat {
    int rowno;
    int colno;
};

program RESERVA_BOL_PROG {
	version RESERVA_BOL_VERS {
		int RESERVE(seat)=1;
        int ROWS(void)=2;
        int COLS(void)=3;
        string LISTFREE(void)=4;
	}=1;
}= 0x31111111;
