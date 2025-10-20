/*

Prerequisites:
 - Vivado XDMA Project with BRAM connected to M_AXI:
   https://github.com/Prandr/XDMA_Tutorial
 - XDMA Drivers from https://github.com/Prandr/dma_ip_drivers


Compile with:

  gcc -Wall mm_axi_over_ioctl_test.c -o mm_axi_over_ioctl_test

Run with:

  sudo ./mm_axi_over_ioctl_test

*/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <xdma_ioctl.h>




// Using 8 kbyte == 8192 byte array. Size was defined in the
// Vivado FPGA Project Block Diagram Address Editor as the Data Range for BRAM
// Each word is 32-Bits so there are 1/4th as many data words.
// On Linux, read/write can transfer at most 0x7FFFF000 (2,147,479,552) bytes
#define DATA_BYTES	8192
#define DATA_WORDS	(DATA_BYTES/sizeof(uint32_t))




int main(int argc, char **argv)
{
	uint32_t write_buffer[DATA_WORDS]={};
	uint32_t read_buffer[DATA_WORDS]={};
	uint64_t address = 0xC0000000;
	int xdma_h2cfd = 0;
	int xdma_c2hfd = 0;
	ssize_t rc;
	//fill the transfer request structure for uploading the data
	struct xdma_transfer_request transfer_request=
	{
		.buf=(char *) write_buffer,//cast is needed
		.length=DATA_BYTES,
		.axi_address=address,
		.mode=XDMA_H2C
	};

	// Fill the write_buffer with data
	for (int i = 0; i < DATA_WORDS; i++) { write_buffer[i] = (DATA_WORDS - i); }

	printf("Buffer Contents before H2C write: \n");
	printf("[0]=%04d, [4]=%04d, [%ld]=%04d\n",
		(uint32_t)write_buffer[0], (uint32_t)write_buffer[4],
		(DATA_WORDS - 3), (uint32_t)write_buffer[(DATA_WORDS - 3)]);

	// Open M_AXI H2C Host-to-Card Device as Write-Only
	xdma_h2cfd = open("/dev/xdma0_h2c_0", O_WRONLY);

	// Write the full write_buffer to the FPGA design's BRAM
	rc = ioctl(xdma_h2cfd, XDMA_IOCTL_SUBMIT_TRANSFER, &transfer_request);
	printf("ioctl returned %zd, errno %i\n", rc, errno);
	//Reuse the structure for downloading data for simplicity. 
	//Probably better use a separate variable in real life applications
	transfer_request.buf=(char *) read_buffer;//cast is neede
	transfer_request.length=DATA_BYTES;//unchanged, redundant
	transfer_request.axi_address=address;//unchanged, redundant
	transfer_request.mode=XDMA_C2H;
	
	// Open M_AXI C2H Card-to-Host Device as Read-Only
	xdma_c2hfd = open("/dev/xdma0_c2h_0", O_RDONLY);

	// Read the full read_buffer from the FPGA design's BRAM
	rc = ioctl(xdma_c2hfd, XDMA_IOCTL_SUBMIT_TRANSFER, &transfer_request);

	printf("\nBuffer Contents after C2H read: \n");
	printf("[0]=%04d, [4]=%04d, [%ld]=%04d\n",
		(uint32_t)read_buffer[0], (uint32_t)read_buffer[4],
		(DATA_WORDS - 3), (uint32_t)read_buffer[(DATA_WORDS - 3)]);

	printf("\nrc = %ld = bytes read from FPGA's BRAM\n", transfer_request.length);


	close(xdma_h2cfd);
	close(xdma_c2hfd);
	exit(EXIT_SUCCESS);
}

