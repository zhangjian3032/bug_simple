# Problem
I was encountered a problem, when I was trying to using dbus synchronized call `new_method_call`
via `asio::connection`, the dbus server sometimes can't receive and response the request.
I found it was happened when the `asio` service is in synchronous calling,
the dbus request is coming, in this case, the dbus server can't receive the request. Also, if
at the same time, another dbus request is coming, the dbus server will handle 2 requests at
the same time.

After debugging, I found the callback `socket.async_read_some`[0] is not called, I don't know
why.
[0]: https://github.com/openbmc/sdbusplus/blob/master/include/sdbusplus/asio/connection.hpp#L324


# Reproduce
I wrote a simple test case to reproduce this problem, the test case is in `case1` directory.

1. Build this case
```
meson build
ninja -C build
```

2. Run the dbus server `fake_server_bar`
Because this bug was happened when the dbus server is in synchronous calling, so we need to
run a dbus server that only provide a service to be called, and this problem will be reproduced
when the dbus server is in synchronous calling, thus for every dbus request, this dbus server
will sleep 1 second to simulate the synchronous calling(actually, it's could happened in any time,
1 second is just for easy to reproduce).

```
~# ./build/case1/fake_server_bar
```

3. Run the dbus server `fake_server`
This server had a timer to sync call the dbus server `fake_server_bar` every 5 second.

```
# using a new terminal.

~# ./build/case1/fake_server
```

4. Run the dbus client `fake_client`
This client will send a dbus request to the dbus server `fake_server` every 10ms,
wait a minute, you will see this bug(hang and timeout exception).

```
# using a new terminal.

~# ./build/case1/fake_client

```

5. Optional: Run the dbus client `busctl --user get-property com.foo /com/foo com.foo Foo`
When this bug was happened, using this command can break the hang and timeout exception.
```
# using a new terminal.
busctl --user get-property com.foo /com/foo com.foo Foo
```

# Others
Because I found the socket callback `socket.async_read_some` is not called, so I tried to
run `read_immediate` after sync call, here's a workaround on this.

