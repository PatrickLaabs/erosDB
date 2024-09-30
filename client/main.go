package main

import (
	"bufio"
	"encoding/base64"
	"fmt"
	"net"
	"os"
	"strings"
)

// Function to encode a file to Base64
func encodeFileToBase64(filePath string) (string, error) {
	fileBytes, err := os.ReadFile(filePath) // Read file contents
	if err != nil {
		return "", err
	}
	// Encode file contents to Base64
	base64Content := base64.StdEncoding.EncodeToString(fileBytes)
	return base64Content, nil
}

// Function to send a PUT command
func sendPutCommand(key string, filePath string) {
	conn, err := net.Dial("tcp", "localhost:3001")
	if err != nil {
		fmt.Println("Error connecting to server:", err)
		return
	}
	defer conn.Close()

	// Encode file content into Base64
	base64Content, err := encodeFileToBase64(filePath)
	if err != nil {
		fmt.Println("Error encoding file:", err)
		return
	}

	base64Length := len(base64Content)

	// Send the PUT command along with key, length, and Base64 content
	command := fmt.Sprintf("PUT %s %d %s", key, base64Length, base64Content)
	fmt.Fprintf(conn, command+"\n")

	// Get response from the server
	response, _ := bufio.NewReader(conn).ReadString('\n')
	fmt.Println("Response from server:", response)
}

// Function to send a GET command
func sendGetCommand(key string) {
	conn, err := net.Dial("tcp", "localhost:3001")
	if err != nil {
		fmt.Println("Error connecting to server:", err)
		return
	}
	defer conn.Close()

	// Send the GET command
	command := fmt.Sprintf("GET %s", key)
	fmt.Fprintf(conn, command+"\n")

	// Read response from the server
	response, _ := bufio.NewReader(conn).ReadString('\n')
	//fmt.Println("Response from server:")
	//fmt.Println(response)

	parts := strings.SplitN(response, " ", 2)
	if len(parts) == 2 {
		encodedContent := strings.TrimSpace(parts[1])
		decodedContent, err := base64.StdEncoding.DecodeString(encodedContent)
		if err != nil {
			fmt.Println("Error decoding Base64 content:", err)
			return
		}

		//fmt.Println("Decoded content:")
		fmt.Println(string(decodedContent))

		filename := key + ".kubeconfig"
		err = os.WriteFile(filename, decodedContent, 0644)
		if err != nil {
			return
		}
	}
}

// Function to send a DELETE command
func sendDeleteCommand(key string) {
	conn, err := net.Dial("tcp", "localhost:8080")
	if err != nil {
		fmt.Println("Error connecting to server:", err)
		return
	}
	defer conn.Close()

	// Send the DELETE command
	command := fmt.Sprintf("DELETE %s", key)
	fmt.Fprintf(conn, command+"\n")

	// Read response from the server
	response, _ := bufio.NewReader(conn).ReadString('\n')
	fmt.Println("Response from server:", response)
}

// Main function
func main() {
	reader := bufio.NewReader(os.Stdin)

	for {
		fmt.Println("Enter command (PUT <key> <filePath>, GET <key>, DELETE <key>):")
		command, _ := reader.ReadString('\n')
		command = strings.TrimSpace(command)

		if strings.HasPrefix(command, "PUT") {
			parts := strings.SplitN(command, " ", 3)
			if len(parts) != 3 {
				fmt.Println("Invalid PUT command format.")
				continue
			}

			key := parts[1]
			filePath := parts[2]

			sendPutCommand(key, filePath) // Now we encode the file
		} else if strings.HasPrefix(command, "GET") {
			parts := strings.SplitN(command, " ", 2)
			if len(parts) != 2 {
				fmt.Println("Invalid GET command format.")
				continue
			}

			key := parts[1]
			sendGetCommand(key)
		} else if strings.HasPrefix(command, "DELETE") {
			parts := strings.SplitN(command, " ", 2)
			if len(parts) != 2 {
				fmt.Println("Invalid DELETE command format.")
				continue
			}

			key := parts[1]
			sendDeleteCommand(key)
		} else {
			fmt.Println("Unknown command.")
		}
	}
}
