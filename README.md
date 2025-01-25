# SSDP BambuLab 3d Printer Discovery

Based on: https://github.com/gashton/bambustudio_tools/blob/master/bambudiscovery.sh and tcpdump.

The code was written quite hastily, with no error or sanity checks.

## Features
- Listens for printer discovery requests via multicast, e.g. from Bambu Studio or OrcaSlicer.
- Sends SSDP responses to emulate a Bambu Lab printer for discovery.
- Allows customization of printer details via a YAML configuration file.
- Allows multiple printers to be 'discovered'.
- Program can be run multiple times with different yaml configs to support multiple NICs.
- No client IP (i.e. machines running OrcaSlicer) configuration necessary, as actual SSDP requests are listened for and replied to.

## Configuration
The program uses a YAML configuration file to define the printer details and network parameters. Below are the parameters you need to adjust in the `config.yaml` file:

### YAML Parameters

- **`listen_address`**
  - Description: The IP address of the machine running this program (must be in the same subnet as Bambu Studio / OrcaSlicer).
  - Example: `192.168.1.x`

- **`usn`**
  - Description: The serial number of the printer. You can find this on your printer or in its settings.
  - Example: `xxxxxxxxxxxxxxx`
  - Reference: [Bambu Lab Wiki - Find SN](https://wiki.bambulab.com/en/general/find-sn)
  - Archived: [Bambu Lab Wiki - Archived](https://archive.is/leSVt)

- **`model`**
  - Description: The model of your printer. Supported values:
    - `3DPrinter-X1-Carbon`
    - `3DPrinter-X1`
    - `C11` (for P1P)
    - `C12` (for P1S)
    - `C13` (for X1E)
    - `N1` (A1 Mini)
    - `N2S` (A1)
  - Example: `N2S`

- **`name`**
  - Description: The friendly name displayed in Bambu Studio or OrcaSlicer. You can set this to any name you prefer.
  - Example: `Bamboozled`

- **`signal`**
  - Description: Fake Wi-Fi signal strength to display.
  - Example: `-44`

- **`address`**
  - Description: The IP address of your printer.
  - Example: `192.168.4.x`

## Usage
1. Install dependencies:
   - Ensure you have `boost-asio` and `yaml-cpp` installed. Use your package manager or build system to include them.

2. Copy and adjust `config.yaml.sample` as `config.yaml` in the same directory as the program.

3. Compile the program using the provided `Makefile`:

   ```bash
   make
   ```

4. Run the program:

   ```bash
   ./ssdp_printer
   ```

Or

   ```bash
   ./ssdp_printer some_other_config.yaml
   ```

## Security Considerations
- Place the printer in a separate VLAN without internet access to ensure isolation.
- Allow only necessary communication from the main VLAN to the printer VLAN.

## License
This program is open-source and distributed under the MIT License. Feel free to modify and use it as needed.
