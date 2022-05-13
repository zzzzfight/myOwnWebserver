**比较重要的几个类：**
* `Acctepor`类
	* 主线程处理的事件的内容（`channel`中注册的回调函数来源） 

* `EPollPoller`类
	* `channels_`:存储事件处理对象
	* `https_`:存储事件处理对象的具体内容，应用层上
	`httpdata_`将回调函数传给`channel_`,`channel`对产生不同的`EPOLL`事件进行响应

* `Timer`类
	* 小根堆计时器，负责超时释放资源
* 线程类
	* 线程池的安全启动，用到了互斥锁，条件变量等

</br>

**主体逻辑:**
1. 主线程(`Acceptor`向`channel`注册回调函数)负责监听外部的事件请求，并建立`TCP/IP`连接。
2. 从`EventLoopThreadPool`中取出（负载均衡（可以这么说吧）采用轮询的方式）`subloop`;
3. 建立事件的实际处理对象{`httpdata`对象和`channel`对象（并注册回调函数）}将对该事件的处理权转交()给`subloop`（对应`loop->queueInLoop(std::bind(&HttpData::newEvent, newHttpConn));`）
4. `EPollPoller`类负责执行epoll事件的处理，以及所有发生的连接的实际处理对象的管理（持有shared_ptr<HttpData>和shared_ptr<Channel>，防止执行回调函数时发生的意外析构）
5. `EventLoop`类 通过Channel对发生的事件进行处理（通过回调函数）删除计时器超时的结点。
6. 对`HttpData`及`Channel`对象（下通称对象）的资源释放发生在两种情况：
	* 超时,计时器`pop() timenode` 析构中执行`handleClose()`
	* `http`解析产生`error`，执行`handleClose()`
	无论是哪种都有一个前提，保证执行回调函数执行期间，对象不会提前析构，那么当在执行回调函数的时候，需要将对对象析构权从计时器释放（`seperaterTimer()`）,`handleConn()`会负责重新在计时器中注册或者直接调用`handleClose()`


</br>

**其他问题**
* 长连接分包如何处理？
完整报文直接处理，不完整报文先读，然后处理

* 如果一个`HTTP`协议内容不完整，然后再次`tcp`传入后在分析，是否保证程序正确性？
	* 逻辑可能存在缺陷，因为解析协议默认认为每次传入的报文是正确的（完整正确，不完整正确）
	* 协议解析思路按行，每次先保证行完整，然后堆行内容解析，如果行不完整，认为报文内容不完整，`inBuffer`状态不改，跳出等待下一次`EPOLLIN`事件,然后将`inBuffer`补完，然后再次解析。

* 网络库的内容大致没问题，`http`功能可能不完善，`post`的解析，或者视频的传输，这些还没解决。

* `http`类 与 `channel`类 的相互引用 可以将`http`对`channel`的引用改为`weak_ptr`,`channel`对`http`的引用改为`shared_ptr`，
然后回调函数将传入的`this` 改为`enable_shared_from()`
这样就不用额外在`EPollPoller`类中用一个 `hashmap`存储`shared_ptr<HttpData>` ,因为`http`的析构只会发生在`channel`的析构时。

</br>

* 写给自己的话
	* 有时间在改进吧，暂时这个项目就告一段落，有很多不满意地方，有很多没有实现的地方，可能也存在一些bug？（http的解析）,比如一个异步日志，比如`http`协议中其他请求的设计，比如把它实现未一个多功能的协议解析服务器？但是接下来的时间想交给刷题和内功。
	* 最后秋招临近，期待自己能够拿到理想的offer。

</br>

* 最后的最后
	* （如果有有缘人）有交流的兴趣，可以邮件我(3161349290@qq.com)
	* 借鉴了很多WebServer/TinyWebServer和muduo库的内容，开始就是从WebServer作为起点，很感谢该作者。
	* 也融入了自己的理解。
	* **如果该项目对你有所帮助，请star一下吧！**