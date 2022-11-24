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

void timerHandler()
{
    static boost::asio::steady_timer timer(io);
    timer.expires_after(std::chrono::seconds(1));
    timer.async_wait([](const boost::system::error_code& ec) {
        if (ec)
        {
            return;
        }
        if (fooProperty == "foo")
        {
            fooProperty = "bar";
        }
        else
        {
            fooProperty = "foo";
        }
        lg2::info("Setting Bar property to {FOO}", "FOO", fooProperty);
        fooIfc->set_property("Bar", fooProperty);
        timerHandler();
    });
}

int main(int /*argc*/, char** /*argv*/)
{

    auto conn = std::make_shared<sdbusplus::asio::connection>(io);
    sdbusplus::asio::object_server objectServer(conn);

    fooIfc = objectServer.add_interface("/com/bar", "com.bar");

    fooIfc->register_property(
        "Bar", std::string("foo"),
        // setter
        [](const std::string& req, std::string& propertyValue) {
            propertyValue = req;
            return 1; // Success
        },
        // getter
        [](const std::string& bar) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            return bar;
        });
    fooIfc->initialize();

    timerHandler();

    conn->request_name("com.bar");

    lg2::info("Bus initialized");
    io.run();

    return 0;
}
