#include "ConnectionListener.h"
#include <csignal>
#include <iostream> 
using namespace boost::asio;
using namespace boost::asio::ip;
using namespace std;

#ifndef SIGPIPE
#define SIGPIPE 13
#endif

twin_star::ConnectionListener::Connection::Connection(ConnectionListener& host, socket_sptr socket) : m_host(host) {
    m_receive_handler   = nullptr;
    m_closed_handler    = nullptr;
    m_socket            = socket;
}

bool twin_star::ConnectionListener::Connection::set_receive_callback(connection_receive_packet_handle handler) {
    if (!m_live_flag) {
        return false;
    }

    m_receive_handler = handler;
    return true;
}

bool twin_star::ConnectionListener::Connection::set_closed_callback(connection_closed_handle handler) {
    if (!m_live_flag) {
        return false;
    }

    m_closed_handler = handler;
    return true;
}
//strearbuf replace string_sptr
void twin_star::ConnectionListener::Connection::send_out_callback() {
    if (!m_live_flag) {
        return;
    }

    m_send_data_list.pop_front();
    if (!m_send_data_list.empty()) {
        m_host.write_data(connection_sptr(this), m_send_data_list.front());
        m_send_ready_flag = false;
    } else {
        m_send_ready_flag = true;
    }
    
}

bool twin_star::ConnectionListener::Connection::send_data(string_sptr const sptr_data) {
    if (!m_live_flag) {
        return false;
    }

    m_send_data_list.push_back(sptr_data);
    if (m_send_ready_flag) {
        m_host.write_data(connection_sptr(this), m_send_data_list.front());
        m_send_ready_flag = false;
    }

    return true;
}

void twin_star::ConnectionListener::Connection::receive_callback() {
    if (!m_live_flag)
        return;

    if (!m_filter_list.empty())
    {
        bool filter_done_flag = false;
        while (m_current_fileter_number < m_filter_list.size())
        {
            filter_done_flag = m_filter_list[m_current_fileter_number](*this, m_receive_buf, m_filter_packet);
            if (filter_done_flag)
                m_current_fileter_number++;
            else
                break;
                
        }
        if (m_current_fileter_number == m_filter_list.size())
            m_current_fileter_number = 0;
        else
            return;
    }

    if (m_receive_handler) {
        string_sptr packet(new string(boost::asio::buffers_begin(m_filter_packet.data()), 
                                        boost::asio::buffers_begin(m_filter_packet.data())));

        m_receive_handler(*this, packet);
    }
}

void twin_star::ConnectionListener::Connection::closed_callback() {
    if (m_live_flag && m_closed_handler) {
        m_closed_handler(*this);
    }
    close_connection();
}

bool twin_star::ConnectionListener::Connection::close_connection() {
    if (!m_live_flag) {
        return false;
    }

    m_live_flag = false;
    m_socket->close();

    m_send_data_list.clear();

    return true;
}

void twin_star::ConnectionListener::Connection::set_error_message(const string& error_message) {
    m_error_message = error_message;
}
string twin_star::ConnectionListener::Connection::get_error_message() {
    return m_error_message;
}

auto twin_star::ConnectionListener::Connection::add_filter(filter_handle filter_handler) -> Connection &
{
    m_filter_list.push_back(filter_handler);
    return *this;
}

void twin_star::ConnectionListener::Connection::reset_buf_and_filter()
{
    m_receive_buf.consume(m_receive_buf.size());
    m_filter_packet.consume(m_filter_packet.size());
    m_current_fileter_number = 0;
}

twin_star::ConnectionListener::~ConnectionListener() {
}

twin_star::ConnectionListener::ConnectionListener(accept_handle accept_handler) :
    m_service_acceptor(m_io_service, tcp::endpoint(tcp::v4(), 8880))
{
    m_accept_handler = accept_handler;
    m_run_flag  = false;
    m_port      = 8880;
}
twin_star::ConnectionListener::ConnectionListener(accept_handle accept_handler, int port) :
    m_service_acceptor(m_io_service, tcp::endpoint(tcp::v4(), port))
{
    m_accept_handler = accept_handler;
    m_run_flag  = false;
    m_port      = port;
}

void twin_star::ConnectionListener::complete_read_callback(connection_sptr connection, const boost::system::error_code& err, size_t bytes_transferred) {
    if (err) {
        connection->set_error_message(err.message());
        connection->closed_callback();
        return;
    }
    connection->m_receive_buf.commit(bytes_transferred);

    connection->receive_callback();

    auto read_callback = boost::bind(&ConnectionListener::complete_read_callback, this, connection, _1, _2);
    connection->m_socket->async_read_some(boost::asio::buffer(connection->m_receive_buf.prepare(255), 255), read_callback);
}

void twin_star::ConnectionListener::complete_write_callback(connection_sptr connection, const boost::system::error_code& err, size_t bytes_transferred) {
    if (err) {
        connection->set_error_message(err.message());
        connection->closed_callback();
        return;
    }
    connection->send_out_callback();
}

void twin_star::ConnectionListener::accept_callback(socket_sptr socket, const boost::system::error_code& err) {
    if (err) {
        m_error_message = err.message();
        return;
    }

    connection_sptr connection = connection_sptr(new Connection(*this, socket));

    m_accept_handler(connection);

    auto handler = boost::bind(&ConnectionListener::complete_read_callback, this, connection, _1, _2);
    connection->m_socket->async_read_some(boost::asio::buffer(connection->m_receive_buf.prepare(255), 255), handler);

    async_accept();
}

void twin_star::ConnectionListener::async_accept() {
    socket_sptr socket = socket_sptr(new tcp::socket(this->m_io_service));

    auto accept_handler = boost::bind(&ConnectionListener::accept_callback, this, socket, _1);

    m_service_acceptor.async_accept(*socket, accept_handler);
}

bool twin_star::ConnectionListener::run_service() {
    if (!m_run_flag) {
        m_run_flag = true;

        signal(SIGPIPE, SIG_IGN);
        async_accept();
        this->m_io_service.run();

        m_run_flag = false;
        return true;
    }
    return false;
}

void twin_star::ConnectionListener::stop_service() {
    m_run_flag = false;
    m_io_service.stop();
}

string twin_star::ConnectionListener::get_error_message() {
    return m_error_message;
}

void twin_star::ConnectionListener::write_data(connection_sptr p_connection, string_sptr const sptr_data) {
    boost::asio::async_write(*(p_connection->m_socket),
        boost::asio::buffer(*(sptr_data.get())),
        boost::asio::transfer_all(),
        boost::bind(&ConnectionListener::complete_write_callback, this, p_connection,
            boost::placeholders::_1,
            boost::placeholders::_2));
}
