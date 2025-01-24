# SSDP BambuLab 3d Printer Discovery

Based on: https://github.com/gashton/bambustudio_tools/blob/master/bambudiscovery.sh and tcpdump.

The code was written quite hastily, with no error or sanity checks.

This program was created to address claims by Bambu Lab that their 3D printers are inherently insecure, necessitating restrictive security measures that limit user control. To accommodate their concerns, this program enables users to discover Bambu Lab printers in a secure network configuration, such as a separate VLAN, without compromising usability.

In this setup, the printer is placed in a separate VLAN without internet access. The main VLAN can access the printer, but the printer cannot initiate communication back. However, certain applications, like OrcaSlicer, require SSDP (Simple Service Discovery Protocol) to find the printer on the network. This program emulates the SSDP responses, enabling OrcaSlicer to discover the printer in such a network configuration.

## Features
- Listens for printer discovery requests via multicast.
- Sends SSDP responses to emulate a Bambu Lab printer for discovery.
- Allows customization of printer details via a YAML configuration file.

## Configuration
The program uses a YAML configuration file to define the printer details and network parameters. Below are the parameters you need to adjust in the `config.yaml` file:

### YAML Parameters

- **`PRINTER_USN`**
  - Description: The serial number of the printer. You can find this on your printer or in its settings.
  - Example: `xxxxxxxxxxxxxxx`
  - Reference: [Bambu Lab Wiki - Find SN](https://wiki.bambulab.com/en/general/find-sn)

- **`PRINTER_MODEL`**
  - Description: The model of your printer. Supported values:
    - `3DPrinter-X1-Carbon`
    - `3DPrinter-X1`
    - `C11` (for P1P)
    - `C12` (for P1S)
    - `C13` (for X1E)
    - `N1` (A1 Mini)
    - `N2S` (A1)
  - Example: `N2S`

- **`PRINTER_NAME`**
  - Description: The friendly name displayed in Bambu Studio or OrcaSlicer. You can set this to any name you prefer.
  - Example: `Bamboozled`

- **`PRINTER_SIGNAL`**
  - Description: Fake Wi-Fi signal strength to display.
  - Example: `-44`

- **`PRINTER_ADDRESS`**
  - Description: The IP address of your printer.
  - Example: `192.168.2.x`

- **`LISTEN_ADDRESS`**
  - Description: The IP address of the machine running this program (must be in the same subnet as the printer).
  - Example: `192.168.1.y`

## Usage
1. Install dependencies:
   - Ensure you have `boost-asio` and `yaml-cpp` installed. Use your package manager or build system to include them.

2. Create a `config.yaml` file in the same directory as the program:

   ```yaml
   PRINTER_USN: "xxxxxxxxxxxxxxx"
   PRINTER_MODEL: "N2S"
   PRINTER_NAME: "Bamboozled"
   PRINTER_SIGNAL: "-44"
   PRINTER_ADDRESS: "192.168.2.x"
   LISTEN_ADDRESS: "192.168.1.y"
   ```

3. Compile the program using the provided `Makefile`:

   ```bash
   make
   ```

4. Run the program:

   ```bash
   ./ssdp_printer
   ```

## Security Considerations
- Place the printer in a separate VLAN without internet access to ensure isolation.
- Allow only necessary communication from the main VLAN to the printer VLAN.

## License
This program is open-source and distributed under the MIT License. Feel free to modify and use it as needed.
