// ---------------------------------------------------------
// Done by Maciej Sawitus (C) 2014
// Feel free to use for any purpose
// I am not responsible for any damage this code might cause
//
// Note: when running in VS you might need to disable
// Edit & Continue for it to compile.
// ---------------------------------------------------------

#include <vector>
#include <assert.h>

namespace Latent
{
	// Simple type helpers

	class Type {};

	template < typename T >
	class ActualType : public Type {};

	template < typename T >
	Type* GetType()
	{
		static ActualType< T > type;
		return &type;
	}

	//! Base variable: function parameter or local variable
	class Var
	{
	public:
		virtual ~Var() {}

		virtual Type* GetType() const = 0;

		inline void* GetValue()
		{
			return this + 1;
		}
	};

	//! Typed variable capable of destructing stored variable
	template < typename T >
	class TypedVar : public Var
	{
	private:
		T value;

	public:
		TypedVar( const T& _value )
			: value( _value )
		{}

		virtual Type* GetType() const
		{
			return Latent::GetType< T >();
		}
	};

	//! Stack frame coresponding with particular function call
	class Frame
	{
	private:
		int state;
		std::vector< Var* > vars;
	public:
		Frame()
			: state( 0 )
		{}
		~Frame()
		{
			for ( unsigned int i = 0; i < vars.size(); ++i )
			{
				delete vars[ i ];
			}
		}
		void SetState( int state )
		{
			this->state = state;
		}
		int GetState() const
		{
			return state;
		}
		unsigned int GetVarCount() const
		{
			return vars.size();
		}
		Var* GetVar( int index ) const
		{
			return vars[ index ];
		}
		void PushVar( Var* var )
		{
			vars.push_back( var );
		}
	};

	//! Full (nested) function call stack
	class Stack
	{
	private:
		std::vector< Frame* > frames;
		unsigned int currentFrame;
		unsigned int currentVar;

	public:
		Stack()
			: currentFrame( -1 )
			, currentVar( -1 )
		{}

		~Stack()
		{
			Reset();
		}

		void Reset()
		{
			for ( unsigned int i = 0; i < frames.size(); ++i )
			{
				delete frames[ i ];
			}
			frames.clear();
		}

		void PushFrame()
		{
			frames.push_back( new Frame() );
		}

		void PopFrame()
		{
			delete frames.back();
			frames.pop_back();
		}

		void IncCurrentFrame()
		{
			++currentFrame;
			currentVar = -1;
		}

		void DecCurrentFrame()
		{
			--currentFrame;
		}

		void SetState( int state )
		{
			frames[ currentFrame - 1 ]->SetState( state );
		}

		int GetState() const
		{
			return frames[ currentFrame - 1 ]->GetState();
		}

		template < typename T >
		T& PushVar( const T& value = T() )
		{
			Var* var = new TypedVar< T >( value );
			frames[ currentFrame ]->PushVar( var );
			return *( T* ) var->GetValue();
		}
		
		template < typename T >
		T& GetVar( int index )
		{
			Var* var = frames[ currentFrame ]->GetVar( index );
			assert( var->GetType() == GetType< T >() );
			return *( T* ) var->GetValue();
		}

		template < typename T >
		T& GetNextVar()
		{
			Var* var = frames[ currentFrame ]->GetVar( ++currentVar );
			assert( var->GetType() == GetType< T >() );
			return *( T* ) var->GetValue();
		}

		template < typename T >
		T& GetOrPushNextVar()
		{
			if ( currentVar + 1 < frames[ currentFrame ]->GetVarCount() )
			{
				return GetNextVar< T >();
			}
			++currentVar;
			return PushVar< T >();
		}
	};

	//! Helper class used to leave current stack frame on function return
	class Scope
	{
	private:
		Stack& stack;

	public:
		Scope( Stack& _stack )
			: stack( _stack )
		{
			stack.IncCurrentFrame();
		}
		~Scope()
		{
			stack.DecCurrentFrame();
		}
	};

	//! User function signature
	typedef bool (*Func)( Stack& stack );

	//! User function call object
	class Call
	{
	private:
		enum State
		{
			State_Initializing,
			State_InProgress,
			State_Done
		};
		Stack stack;
		Func func;
		State state;

	public:
		Call()
			: func( NULL )
			, state( State_Initializing )
		{
			stack.PushFrame();
			stack.IncCurrentFrame();
		}
		void Reset() { stack.Reset(); state = State_Initializing; func = NULL; }
		bool IsDone() const { return state == State_Done; }
		void Do( Func func ) { this->func = func; Advance(); }
		void Advance() { state = func( stack ) ? State_Done : State_InProgress; }

		template < typename T >
		void PushParam( const T& var = T() ) { stack.PushVar< T >( var ); }
		template < typename T >
		T& GetParam( int index ) { return stack.GetVar< T >( index ); }
	};
};	// namespace Latent

// Miscellaneous macros implementing latent function call functionality

#define LatentFunc( name ) bool name( Latent::Stack& stack )

#define LatentParam( type, name ) type& name = stack.GetNextVar< type >();

#define LatentLocal( type, name ) type& name = stack.GetOrPushNextVar< type >();

#define LatentBegin()					\
	Latent::Scope latentScope( stack );	\
	switch ( stack.GetState() )			\
	{									\
	case 0:

#define LatentEnd()	\
	}				\
	return true;

#define LatentReturn( isDone )						\
	do { stack.SetState( __LINE__ ); return isDone; \
    case __LINE__:; } while (0)

#define LatentCallBegin() stack.PushFrame();

#define LatentCallPushParamTyped( type, value ) stack.PushVar< type >( value );
#define LatentCallPushParam( value ) stack.PushVar( value );

#define LatentCallDo( func )			\
	do									\
	{									\
		stack.SetState( __LINE__ );		\
		case __LINE__:;					\
		if ( !func( stack ) )			\
		{								\
			return false;				\
		}								\
	} while (0)

#define LatentCallEnd() stack.PopFrame();

// Convenience API for a special case of 1-parameter latent function call without return result

#define LatentCallDo1Arg( func, param0 )	\
	LatentCallBegin();						\
	LatentCallPushParam( param0 );			\
	LatentCallDo( func );					\
	LatentCallEnd();
