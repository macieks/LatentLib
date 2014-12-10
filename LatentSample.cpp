// ---------------------------------------------------------
// Done by Maciej Sawitus (C) 2014
// Feel free to use for any purpose
// I am not responsible for any damage this code might cause
//
// Note: when running in VS you might need to disable
// Edit & Continue for it to compile.
// ---------------------------------------------------------

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "LatentLib.h"

using namespace std;

void SleepSecs( float secs )
{
	::Sleep( ( int ) ( secs * 1000.0f ) );
}

// Blocking sample

int SomeFunc( const vector< int > a )
{
	unsigned int i;
	int sum;

	sum = 0;
	for ( i = 0; i < a.size(); ++i )
	{
		sum += a[ i ];
		printf( "sum = %d\n", sum );
		SleepSecs( 1.0f );
	}

	printf( "DONE\n" );
	SleepSecs( 2.0f );

	return sum * sum;
}

void BlockingSample()
{
	printf("Blocking sample:\n");

	vector< int > v;
	v.push_back( 1 );	
	v.push_back( 2 );
	v.push_back( 3 );
	const int sum = SomeFunc( v );
}

// Non-blocking sample

const float g_deltaTime = 0.1f;

LatentFunc( SleepSecsLatent )
{
	LatentParam( float, secs );

	LatentBegin();
	LatentReturn( secs <= 0.0f );
	while ( 1 )
	{
        secs -= g_deltaTime;
		LatentReturn( secs <= 0.0f );
	}
	LatentEnd();
}

LatentFunc( SomeFuncLatent )
{
	LatentParam( int, result );
	LatentParam( vector< int >, a );
	LatentLocal( unsigned int, i );
	LatentLocal( int, sum );

	LatentBegin();

	sum = 0;
	for ( i = 0; i < a.size(); ++i )
	{
		sum += a[ i ];
		printf( "sum = %d\n", sum );
		LatentCallDo1Arg( SleepSecsLatent, 1.0f );
	}

	printf( "DONE\n" );
	LatentCallDo1Arg( SleepSecsLatent, 2.0f );

	result = sum * sum;

	LatentEnd();
}

void NonBlockingSample()
{
	printf("Non-blocking sample:\n");

	vector< int > v;
	v.push_back( 1 );	
	v.push_back( 2 );
	v.push_back( 3 );

	// Set up call

	Latent::Call call;
	call.PushParam< int >();// Function result will be stored here
	call.PushParam( v );	// First function parameter
	
	// Begin the call
	
	call.Do( SomeFuncLatent );

	// Keep resuming the call until done

	while ( !call.IsDone() )
	{
		call.Advance();
		SleepSecs( g_deltaTime );
	}

	// Grab call result

	const int result2 = call.GetParam< int >( 0 );
}

// Main

void main()
{
	BlockingSample();
	NonBlockingSample();
}
