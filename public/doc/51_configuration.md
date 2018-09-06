# Configuration

## User ID and Group ID

The audio related components like e.g. the SmartXbar or the avb_streamhandler create some files during runtime, like e.g. named pipes. To limit the access to these files, the user id *ias\_audio* and the corresponding group id *ias\_audio* are introduced by the ias-audio-common-config package. 
The user id, that is used to start audio applications, like e.g. a media player, has to be added to the group *ias\_audio* for being able to access the files created by the SmartXbar or the avb_streamhandler.

## The Runtime Directory

The runtime directory <em>/run/smartx</em> for hosting the named pipes is also created by the ias-audio-common-config package. The user and group id of this directory is set to *ias\_audio*. The *GUID* flag is set, so that everything created below this directory will inherit the group id *ias\_audio*.

To create the runtime directory, the systemd tmpfile functionality is used. This part is located in the meta-ias-audio layer in the recipe ias-audio-common-config.bb.    