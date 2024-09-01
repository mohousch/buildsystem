#ifdef __KERNEL__
int fake_avs_init(struct i2c_client *client);
int fake_avs_command(struct i2c_client *client, unsigned int cmd, void *arg );
int fake_avs_command_kernel(struct i2c_client *client, unsigned int cmd, void *arg);
#endif

