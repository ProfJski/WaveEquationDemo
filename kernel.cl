void kernel sinusoidal(const float time, const float amplitude, global float* vel, global float* pos, global float* offset) {
    int gid=get_global_id(0);
    pos[gid]=(amplitude*sin(vel[gid]*time))+offset[gid];
}

void kernel basic(float timestep, global float* vel, global float* pos, global float* mass) {
    int gid=get_global_id(0);
    float force=-0.000001*pos[gid]*pos[gid]*pos[gid];
    //printf("gid: %i pos: %f force: %f",gid, pos[gid], force);
    vel[gid]=vel[gid]+force;
    pos[gid]=pos[gid]+vel[gid]*timestep;
}
