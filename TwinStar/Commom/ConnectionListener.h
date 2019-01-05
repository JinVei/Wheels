#ifndef __CONNECTION_LISTENER_H_
#define __CONNECTION_LISTENER_H_

#include <boost/asio.hpp>
#include <boost/bind.hpp>

namespace twin_star {
    using socket_sptr       = std::shared_ptr<boost::asio::ip::tcp::socket>;
    using socket_wptr       = std::weak_ptr<boost::asio::ip::tcp::socket>;
    using string_sptr       = std::shared_ptr<std::string>;

    enum class return_code: int{
        ok      = 0,
        error   = 1,
        dead    = 2
    };
    //no thread safe
    class ConnectionListener {
    public:
        class Connection;
        using connection_sptr = std::shared_ptr<Connection>;
        using connection_wptr = std::weak_ptr<Connection>;
        using accept_handle   = std::function<void(connection_sptr)>;

    private:
        boost::asio::io_service             m_io_service;
        boost::asio::ip::tcp::acceptor      m_service_acceptor;
        accept_handle                       m_accept_handler;
        int                                 m_port;
        bool                                m_run_flag;
        std::string                         m_error_message;

    private:
        void async_accept();
        void accept_callback(socket_sptr sock, const boost::system::error_code& err);
        void complete_write_callback(connection_sptr Connect, const boost::system::error_code& err, size_t bytes_transferred);
        void complete_read_callback(connection_sptr Connect, const boost::system::error_code& err, size_t bytes_transferred);
    public:
        ~ConnectionListener();
        ConnectionListener(accept_handle accept_handler);
        ConnectionListener(accept_handle accept_handler, int port);
        bool run_service();
        void stop_service();
        auto get_error_message() ->std::string;
        void write_data(connection_sptr socket, string_sptr const sptr_data);
    };

    //no thread safe
    class ConnectionListener::Connection {
    public:
        using connection_closed_handle          = std::function<void(Connection& connection)>;
        using connection_receive_packet_handle  = std::function<void(Connection& connection, string_sptr)>;
        using filter_handle                     = std::function<bool(Connection&, boost::asio::streambuf&)>;

    private:
        friend class ConnectionListener;
        ConnectionListener&              m_host;
        socket_sptr                      m_socket;

        connection_receive_packet_handle m_receive_handler;
        connection_closed_handle         m_closed_handler;

        boost::asio::streambuf           m_receive_buf;
        bool                             m_live_flag        = true;
        std::string                      m_error_message;
        std::list<string_sptr>           m_send_data_list;
        bool                             m_send_ready_flag  = true;
        std::vector<filter_handle>       m_filter_list;
        size_t                           m_current_fileter_number = 0;

    private:
        Connection(ConnectionListener& host, socket_sptr socket);
        void send_out_callback();
        void receive_callback();
        void closed_callback();
        void set_error_message(const std::string& error_message);

    public:
        bool send_data(string_sptr const sptr_data);
        bool set_receive_callback(connection_receive_packet_handle handler);
        bool set_closed_callback(connection_closed_handle handler);
        bool close_connection();
        auto get_error_message() -> std::string;
        auto add_filter(filter_handle filter_handler)->Connection &;
        void reset_buf_and_filter();

    public:
        void* user_data = nullptr;
    };
}

#endif//__CONNECTION_LISTENER_H_
