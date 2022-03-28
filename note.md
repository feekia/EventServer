
IdleTaskPtr

需要考虑循环引用问题，涉及类 TcpConnection（持有IdleTaskPtr，该类fd close 的时候，需要将IdleTask 置为canceled状态，并将回调清除） IoOps（列表持有IdleTaskPtr） IdleTaskPtr(回调持有TcpConnection)