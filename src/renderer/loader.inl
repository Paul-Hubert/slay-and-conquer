#define INST_LOAD(name) \
PFN_##name name = (PFN_##name) ( win->inst.getInstanceProcAddr(#name)); \
assert( name != nullptr);

#define DEV_LOAD(name) \
PFN_##name name = (PFN_##name) ( win->vki->vkGetDeviceProcAddr(static_cast<VkDevice> (win->device.logical),#name)); \
assert( name != nullptr);
