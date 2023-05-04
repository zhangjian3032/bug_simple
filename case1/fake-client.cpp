#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/steady_timer.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/bus/match.hpp>

#include <chrono>
#include <iostream>
#include <variant>

std::shared_ptr<sdbusplus::asio::dbus_interface> fooIfc;
static std::string fooProperty = "foo";
boost::asio::io_context io;
auto conn = std::make_shared<sdbusplus::asio::connection>(io);

template <typename Callback>
void asyncCall(Callback&& cb)
{
    // busctl call xyz.openbmc_project.Ipmi.Channel.Ipmb
    // /xyz/openbmc_project/Ipmi/Channel/Ipmb org.openbmc.Ipmb sendRequest
    // yyyyay 0x01 0x0a 0x00 0x48 0

    constexpr auto service = "xyz.openbmc_project.Ipmi.Channel.Ipmb";
    constexpr auto path = "/xyz/openbmc_project/Ipmi/Channel/Ipmb";
    constexpr auto interface = "org.openbmc.Ipmb";
    constexpr auto method = "sendRequest";
    std::vector<uint8_t> data;

    // iyyyyay
    conn->async_method_call(
        [cb = std::forward<Callback>(cb)](
            const boost::system::error_code ec,
            std::tuple<int, uint8_t, uint8_t, uint8_t, uint8_t,
                       std::vector<uint8_t>>) {
            if (ec)
            {
                lg2::error("asyncCall: failed, ec={EC}", "EC", ec.message());
                return;
            }

            lg2::info("Async call succeeded");

            cb();
        },
        service, path, interface, method, uint8_t(0x01), uint8_t(0x0a),
        uint8_t(0x00), uint8_t(0x48), data);
}

void syncCall()
{
    constexpr auto service = "com.foo";
    constexpr auto path = "/com/foo";
    constexpr auto interface = "com.foo";
    constexpr auto property = "Foo";
    std::variant<std::string> msg;

    auto& bus = *conn;
    auto method = bus.new_method_call(service, path,
                                      "org.freedesktop.DBus.Properties", "Get");
    method.append(interface, property);
    auto reply = bus.call(method);
    reply.read(msg);

    auto now = std::chrono::system_clock::now();
    lg2::info("{NOW} Sync Get Foo property to {FOO}", "NOW",
              std::chrono::system_clock::to_time_t(now), "FOO",
              std::get<std::string>(msg));
    // if fail, throw exception, don't catch, will exit
}

void syncTimer()
{
    static boost::asio::steady_timer timer(io);
    timer.expires_after(std::chrono::milliseconds(100));
    timer.async_wait([](const boost::system::error_code& ec) {
        if (ec)
        {
            return;
        }

        asyncCall([]() {});
        syncTimer();
    });
}

int main(int /*argc*/, char** /*argv*/)
{
    syncTimer();

    io.run();
    return 0;
}
