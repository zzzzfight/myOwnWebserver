// @Author Z
// @Email 3161349290@qq.com
#include "Acceptor.h"
#include "EventLoop.h"

int main()
{
	EventLoop mainLoop;
	Acceptor acceptor(&mainLoop, 1, 9006);
	acceptor.start();
	mainLoop.loop();
}