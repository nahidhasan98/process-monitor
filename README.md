# process-monitor


### Task:
Monitor applications/processes in Linux

### Requirements:

##### The configuration requirements:

- The watchdog configuration should be read on startup from the plain text file (any common configuration format is acceptable: YAML, JSON, INI, etc.)

- The configuration should be reloaded dynamically when the changes are made in the configuration file

- The configuration file should describe the list of processes/applications which need to be monitored by watchdog

- Each monitored process/application in the configuration file can have startup parameters

##### The processing requirements:

- All the configured processes/applications should be checked on startup and started with the configured startup parameters if they are not running already

- If a new process/application is added into the configuration file after the watchdog is already started - the watchdog should dynamically check and start such new process/application with the configured startup parameters

- If one of the configured processes/applications is stopped or killed - it should be automatically restarted by watchdog with the configured startup parameters

- All the processes/applications removed from the configuration file after the watchdog is already started should no longer be monitored and restarted by the watchdog (until such processes/applications are added back to the configuration file)

- The process/application monitoring should be implemented using system events or periodic checks or both

##### All the Native OS API should be implemented as a separate component

- This component should encapsulate all the Native API details and provide a simple interface for all the actions

- There should not be any other way to access the Native OS API, only through this component

- It should be possible to implement support of other operating systems (for example support Windows in addition to Linux) by extending the component
