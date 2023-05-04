#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/bus/match.hpp>

#include <chrono>
#include <thread>

std::shared_ptr<sdbusplus::asio::dbus_interface> fooIfc;
static std::string fooProperty = "foo";
boost::asio::io_context io;
auto conn = std::make_shared<sdbusplus::asio::connection>(io);

void syncCall()
{
    constexpr auto service = "com.bar";
    constexpr auto path = "/com/bar";
    constexpr auto interface = "com.bar";
    constexpr auto property = "Bar";

    std::variant<std::string> msg;

    auto& bus = *conn;
    auto method = bus.new_method_call(service, path,
                                      "org.freedesktop.DBus.Properties", "Get");
    method.append(interface, property);
    auto start = std::chrono::steady_clock::now();
    fprintf(stderr, "Debug sync call start: %lld\n",
            std::chrono::duration_cast<std::chrono::milliseconds>(
                start.time_since_epoch())
                .count());
    bus.call(method);
    auto end = std::chrono::steady_clock::now();
    fprintf(stderr, "Debug sync call end: %lld\n",
            std::chrono::duration_cast<std::chrono::milliseconds>(
                end.time_since_epoch())
                .count());
}

template <typename Callback>
void asyncCall(Callback&& cb)
{
    constexpr auto service = "com.bar";
    constexpr auto path = "/com/bar";
    constexpr auto interface = "com.bar";
    constexpr auto property = "Bar";

    conn->async_method_call(
        [cb =
             std::forward<Callback>(cb)](const boost::system::error_code ec,
                                         const std::variant<std::string>& msg) {
            if (ec)
            {
                lg2::error("asyncCall: Get Foo property failed");
                exit(1);
                return;
            }
            lg2::info("Async Get Foo property to {FOO}", "FOO",
                      std::get<std::string>(msg));
            cb();
        },
        service, path, "org.freedesktop.DBus.Properties", "Get", interface,
        property);
}

void timerHandler()
{
    static boost::asio::steady_timer timer(io);
    timer.expires_after(std::chrono::seconds(5));
    timer.async_wait([](const boost::system::error_code& ec) {
        if (ec)
        {
            return;
        }

#if 1
        syncCall();
        timerHandler();
#else
        asyncCall([]() { timerHandler() });
#endif
    });
}

int main(int /*argc*/, char** /*argv*/)
{
    sdbusplus::asio::object_server objectServer(conn);

    fooIfc = objectServer.add_interface("/com/foo", "com.foo");

    fooIfc->register_property(
        "Foo", std::string("foo"),
        // setter
        [](const std::string& req, std::string& propertyValue) {
            propertyValue = req;
            return 1; // Success
        },
        // getter
        [](const std::string& foo) {
            // lg2::info("Getting Foo property to {FOO}", "FOO", foo);
            return foo;
        });
    fooIfc->initialize();

    timerHandler();

    conn->request_name("com.foo");

    lg2::info("Bus initialized");
    io.run();

    return 0;
}
