void A ( void );
void B ( void );
void C ( void );

int main( void )
{
	void (*a)(void);
	a = A;
	(*a)();

	return 0;
}

void A ( void )
{
	B( );
}

void B ( void )
{
	C( );
}

void C ( void )
{
	A( );
}
