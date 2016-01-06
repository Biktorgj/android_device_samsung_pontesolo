/** DUMMY SENSOR **/

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <cutils/log.h>
#include <cstring>

/*** Predefined data */

#include "GeoMagneticSensor.h"
#define LOGTAG "MagneticSensor"


/********************/

MagneticSensor::MagneticSensor()
    : SensorBase(NULL, "geomagnetic_sensor"),
      mEnabled(0),
      mInputReader(4),
      mHasPendingEvent(false),
      mEnabledTime(0)
	{

	mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = ID_M;
    mPendingEvent.type = SENSOR_TYPE_MAGNETIC_FIELD;
    memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));
	
    if (data_fd) {
        strcpy(input_sysfs_path, "/sys/class/input/");
        strcat(input_sysfs_path, input_name);
        strcat(input_sysfs_path, "/device/");
        input_sysfs_path_len = strlen(input_sysfs_path);
    }
}


MagneticSensor::~MagneticSensor() {

    if (mEnabled) {
		ALOGW("MAGNETIC_SENSOR: ~MagneticSensor()");
        enable(0, 0);
    }
}

int MagneticSensor::setInitialState()
{
	ALOGW("MAGNETIC_SENSOR: Set Initial State");
    struct input_absinfo absinfo_x;
    struct input_absinfo absinfo_y;
    struct input_absinfo absinfo_z;
    float value;
    if (!ioctl(data_fd, EVIOCGABS(REL_RX), &absinfo_x) &&
        !ioctl(data_fd, EVIOCGABS(REL_RY), &absinfo_y) &&
        !ioctl(data_fd, EVIOCGABS(REL_RZ), &absinfo_z)) {
		
		mPendingEvent.data[0] = value * CONVERT_M;// * CONVERT_GYRO_X;
        value = absinfo_x.value;
        mPendingEvent.data[1] = value * CONVERT_M;// * CONVERT_GYRO_Y;
        value = absinfo_x.value;
        mPendingEvent.data[2] = value * CONVERT_M;// * CONVERT_GYRO_Z;
        mHasPendingEvent = true;
    }
    return 0;
}

int MagneticSensor::enable(int32_t handle, int en) {
	int flags = en ? 1 : 0;
    int err;
    if (flags != mEnabled) {
	ALOGE("ENABLE MAGNETIC SENSOR");
    err = sspEnable(LOGTAG, SSP_MAG, en);
          if(err >= 0){
             mEnabled = flags;
             setInitialState();

             return 0;
         }
         return -1;
    }
    return 0; 
}


bool MagneticSensor::hasPendingEvents() const {
   return mHasPendingEvent;
}


int MagneticSensor::setDelay(int32_t handle, int64_t ns)
{
    int fd;
    /*if (ns < 10000000) {
        ns = 10000000; // Minimum on stock
    }*/
    strcpy(&input_sysfs_path[input_sysfs_path_len], "mag_poll_delay");
    fd = open(input_sysfs_path, O_RDWR);
    if (fd >= 0) {
        char buf[80];
        sprintf(buf, "%lld", ns);
        write(fd, buf, strlen(buf)+1);
        close(fd);
        return 0;
    }
    return -1;
}


int MagneticSensor::readEvents(sensors_event_t* data, int count)
{
    if (count < 1)
        return -EINVAL;

    if (mHasPendingEvent) {
        mHasPendingEvent = false;
        mPendingEvent.timestamp = getTimestamp();
        *data = mPendingEvent;
        return mEnabled ? 1 : 0;
    }

    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
        return n;
    int numEventReceived = 0;
    input_event const* event;

    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
		// no fucking idea if this fucking shit will work. 
        if (type == EV_REL && event->value > 0 && event->value < 360 ) {
            float value = event->value;
			/* ACTUAL DATA PROCESSING GOES HERE */
			switch (event->code) {
				case REL_RX:
					ALOGE("MAGNETIC_SENSOR: %i REL_RX", event->value);
					 mPendingEvent.data[0] = value * CONVERT_M;
					break;
				case REL_RY:
					ALOGE("MAGNETIC_SENSOR: %i REL_RY", event->value);
					 mPendingEvent.data[1] = value * CONVERT_M;
					break;
				case REL_RZ:
					ALOGE("MAGNETIC_SENSOR: %i REL_RZ", event->value);			
					 mPendingEvent.data[2] = value * CONVERT_M;
					break;
			}
		}else if (type == EV_SYN) {
            mPendingEvent.timestamp = timevalToNano(event->time);
            if (mEnabled) {
                if (mPendingEvent.timestamp >= mEnabledTime) {
                    *data++ = mPendingEvent;
                    numEventReceived++;
                }
                count--;
            }
        } else {
            ALOGE("%s: unknown event (type=%d, code=%d)", LOGTAG,
                    type, event->code);
        }

        mInputReader.next();
    }
#if FETCH_FULL_EVENT_BEFORE_RETURN
    /* if we didn't read a complete event, see if we can fill and
       try again instead of returning with nothing and redoing poll. */
    if (numEventReceived == 0 && mEnabled == 1) {
        n = mInputReader.fill(data_fd);
        if (n)
            goto again;
    }
#endif
	
    return numEventReceived;

}
