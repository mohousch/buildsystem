#ifdef __KERNEL__
int avs_none_init(struct i2c_client *client);
int avs_none_command(struct i2c_client *client, unsigned int cmd, void *arg );
int avs_none_command_kernel(struct i2c_client *client, unsigned int cmd, void *arg);
#endif

