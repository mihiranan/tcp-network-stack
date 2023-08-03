**`CS 144: Computer Networking`**
## Homegrown Networking Stack – A Comprehensive Overview

This project is focused on the implementation of a TCP receiver, sender, and the overall connections object, it allows for reliable bidirectional data transmission with existing versions of TCP.

### TCP: Transmission Control Protocol
TCP forms one of the core protocols of the Internet protocol suite. It operates at the transport layer and provides a reliable, ordered, and error-checked delivery of a stream of bytes. It ensures that data packets are reassembled in the order in which they were sent and handles lost or duplicated packets.

### Key Components

- **`TCP Receiver`:** Responsible for receiving data segments, it ensures proper ordering and handles duplicate segments. Plays a vital role in the acknowledgment mechanism.

- **`TCP Sender`:** Manages the transmission of data segments, including handling retransmissions in case of lost packets. It uses algorithms like sliding window to control the flow of data.

- **`Connections Object`:** Acts as a bridge between the receiver and sender, tying them together to ensure smooth, reliable communication. It coordinates the various aspects of TCP, such as connection establishment and termination.

### Features of the Networking Stack

* **Reliable Data Transmission:** Ensures that the data is transferred accurately and in order, handling any network inconsistencies.
* **Bidirectional Communication:** Allows for two-way communication between the sender and receiver, using the established TCP connection.
* **Integration with Existing TCP Versions:** Compatible with existing versions of TCP, allowing for interoperability with other TCP implementations.

---

## Reflection

### Design Considerations
* **Scalability:** Designed to handle varying network loads, accommodating different scenarios and applications.
* **Efficiency:** Focuses on optimized algorithms and methods that enable fast and reliable data transmission.
* **Reliability:** Implementation emphasizes error handling and robustness, ensuring dependable communication.

### Strengths:
- **Comprehensive Understanding of Networking Principles:** Offers an in-depth exploration of packet switching, layering, encapsulation, and other fundamental networking concepts.
- **Hands-On Experience:** Building essential parts of the Internet infrastructure provides unique insights into how the Internet works, including reliable communication over an unreliable Internet.

### Weaknesses:
- **Potential Limitations in Compatibility:** As a homegrown solution, there might be specific scenarios or edge cases that are not fully supported.

### Conclusion
The "Homegrown Networking Stack" project encapsulates the fundamental principles of computer networks as taught in CS 144. By implementing key aspects of the TCP protocol, including the receiver, sender, and connections object, it provides a practical and enriching experience into the world of networking. The project stands as an illustration of the complexities of data transmission and the innovative solutions that make the Internet the largest computer network ever built.

