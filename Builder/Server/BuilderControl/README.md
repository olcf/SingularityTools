# Server Control

To Begin download an OpenStack RC file onto a system on the ORNL network
* Login to `cloud.cades.ornl.gov`
* Navigate to `Compute -> Access & Security -> API Access`
* Click `Download OpenStack RC File v3`
* Rename downloaded file as openrc.sh and move it to `SingularityTools/Builder/Server/BuilderControl`

To bring up a new builder instance:
```
SingularityTools/Builder/Server/BuilderControl/BringUp
```
After bringup three files will be created containing the Builder IP address as well as two SSH keys. `CadesKey` provides access for the cades user for administration of the builder while `BuilderKey` provides access the builder user who is restricted to running `SafeSH`. `BuilderIP` will contain the builders IP address.

To destroy a new builder instance:
```
SingularityTools/Builder/Server/BuilderControl/TearDown
```
