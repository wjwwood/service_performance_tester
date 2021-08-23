#include <chrono>
#include <fstream>
#include <memory>
#include <vector>

#include <rclcpp/rclcpp.hpp>
#include <rclcpp_components/register_node_macro.hpp>
#include <std_srvs/srv/empty.hpp>

class ServiceRequestRecord
{
public:
  // The timestamp taken by the middleware when sending the request on the remote side.
  std::chrono::system_clock::time_point source_timestamp;
  // The timestamp taken by the middleware when the request was received.
  std::chrono::system_clock::time_point destination_timestamp;
  // The timestamp taken by this node when the request was delivered to the callback.
  std::chrono::system_clock::time_point callback_timestamp;
  // The timestamp taken just before the request is sent, by returning from the callback.
  std::chrono::system_clock::time_point response_sent_timestamp;

  static
  std::string
  csv_header()
  {
    return "source timestamp, destination timestamp, callback timestamp, response sent timestamp";
  }

  static
  std::string
  timestamp_to_string(std::chrono::system_clock::time_point timestamp)
  {
    return std::to_string(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
        timestamp.time_since_epoch()
      ).count()
    );
  }

  std::string
  to_csv() const
  {
    return (
      timestamp_to_string(source_timestamp) +
      timestamp_to_string(destination_timestamp) +
      timestamp_to_string(callback_timestamp) +
      timestamp_to_string(response_sent_timestamp)
    );
  }
};

class InstrumentedServiceNode : public rclcpp::Node
{
public:
  explicit InstrumentedServiceNode(rclcpp::NodeOptions node_options)
  : rclcpp::Node("instrumented_service_node", node_options),
    start_time_(std::chrono::system_clock::now())
  {
    service_ = this->create_service<std_srvs::srv::Empty>(
      "empty",
      [this](
        std::shared_ptr<rmw_request_id_t> request_id,
        const std::shared_ptr<const std_srvs::srv::Empty::Request> request,
        std::shared_ptr<std_srvs::srv::Empty::Response> response)
      {
        auto now = std::chrono::system_clock::now();
        service_request_records_.push_back({
          {},
          {},
          now,
          {}
        });
        service_request_records_.back().response_sent_timestamp = std::chrono::system_clock::now();
      }
    );
  }

  virtual ~InstrumentedServiceNode()
  {
    std::ofstream file;
    std::string node_name = this->get_fully_qualified_name();
    std::replace(node_name.begin(), node_name.end(), '/', '_');
    auto start_time_time_t = std::chrono::system_clock::to_time_t(start_time_);
    std::stringstream start_time_str;
    start_time_str << std::put_time(std::localtime(&start_time_time_t), "%Y.%m.%d-%H:%M:%S");
    std::string file_name =
      std::string("records__") + node_name + "__" + start_time_str.str() + ".csv";
    printf("Writing output file '%s'...\n", file_name.c_str());
    file.open(file_name);
    file << ServiceRequestRecord::csv_header() << std::endl;
    for (const auto & record : service_request_records_) {
      file << record.to_csv() << std::endl;
    }
    file.close();
  }

private:
  std::chrono::system_clock::time_point start_time_;
  std::shared_ptr<rclcpp::Service<std_srvs::srv::Empty>> service_;
  std::vector<ServiceRequestRecord> service_request_records_;
};

RCLCPP_COMPONENTS_REGISTER_NODE(InstrumentedServiceNode)
