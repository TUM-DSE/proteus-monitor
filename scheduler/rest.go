package main

import (
	"fmt"
	"io/ioutil"
	"log"
	"net"
	"net/http"
	"strconv"
	"strings"
)

const SockAddr = "/tmp/front.sock"

func uploadFile(w http.ResponseWriter, r *http.Request) {
	fmt.Println("File Upload Endpoint Hit")

	// Parse our multipart form, 10 << 20 specifies a maximum
	// upload of 10 MB files.
	r.ParseMultipartForm(1000 << 20)
	// FormFile returns the first file for the given key `myFile`
	// it also returns the FileHeader so we can get the Filename,
	// the Header and the size of the file

	arguments := r.FormValue("Arguments")

	priorityString := r.FormValue("Priority")
	priority, err := strconv.Atoi(priorityString)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	fmt.Printf("Submitted a task with Priority: %d\n", priority)

	frequencies := r.MultipartForm.Value["frequency"]
	files := r.MultipartForm.File["bitstream"]
	var fileNames []string
	var frequencyValues []string

	for index, fileHeader := range files {
		file, err := fileHeader.Open()
		if err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}
		defer file.Close()

		fmt.Printf("Uploaded File: %+v\n", fileHeader.Filename)
		fmt.Printf("File Size: %+v\n", fileHeader.Size)
		fmt.Printf("MIME Header: %+v\n", fileHeader.Header)
		fmt.Printf("Frequency: %v\n", frequencies[index])

		// Create a temporary file within our temp-images directory that follows
		// a particular naming pattern
		tempFile, err := ioutil.TempFile("/tmp/", "upload-*.ukvm")
		if err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}
		defer tempFile.Close()

		// read all of the contents of our uploaded file into a
		// byte array
		fileBytes, err := ioutil.ReadAll(file)
		if err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}
		// write this byte array to our temporary file
		tempFile.Write(fileBytes)

		fileNames = append(fileNames, tempFile.Name())
		frequencyValues = append(frequencyValues, frequencies[index])
	}

	fmt.Fprintf(w, "Successfully Uploaded File(s)\n")

	c, err := net.Dial("unix", SockAddr)
	if err != nil {
		log.Fatal("Dial error", err)
	}

	// TODO: Wouldn't it be better to serialize this?
	s := fmt.Sprintf("New: %v num_bitstreams: %v frequencies: %v priority: %d args: %v", strings.Join(fileNames, ","), len(fileNames), strings.Join(frequencies, ","), priority, arguments)
	fmt.Printf(s)
	_, err = c.Write([]byte(s))
	if err != nil {
		log.Fatal("Write error", err)
	}
	defer c.Close()
}

func serveIndex(w http.ResponseWriter, r *http.Request) {
	w.Header().Add("Content-Type", "text/html")
	http.ServeFile(w, r, "index.html")
}

func setupRoutes() {
	http.HandleFunc("/", serveIndex)
	http.HandleFunc("/upload", uploadFile)
	http.ListenAndServe(":8080", nil)
}

func main() {
	fmt.Println("HTTP Upload Server")
	setupRoutes()
}
