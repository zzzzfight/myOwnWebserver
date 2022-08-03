#include "Logging.h"

int main()
{

	for (int i = 0; i < 100; i++)
	{

		DEFAULT << "helloworld";
		WARN << "WARN";
		ERROR << "ERROR";
		DEBUG << "DEBUG";
	}
	// DEFAULT<<"DEFAULT";
	stop_thread();
}