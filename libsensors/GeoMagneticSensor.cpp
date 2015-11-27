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
    : SensorBase(NULL, "magnetic_sensor"),
      mEnabled(0),

      mInputReader(4),
      mHasPendingEvent(false)
{
	ALOGW("MAGNETIC SENSOR: Open Input Path");
    //mPendingEvent.version = sizeof(sensors_event_t);
    //mPendingEvent.sensor = ID_M;
    //mPendingEvent.type = SENSOR_TYPE_MAGNETIC_FIELD;
	//mPendingEvent.status = SENSOR_STATUS_ACCURACY_HIGH;
    //memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));
	memset(mPendingEvents, 0, sizeof(mPendingEvents));
	mPendingEvents[MagneticField].version = sizeof(sensors_event_t);
    mPendingEvents[MagneticField].sensor = ID_M;
    mPendingEvents[MagneticField].type = SENSOR_TYPE_MAGNETIC_FIELD;
    mPendingEvents[MagneticField].magnetic.status = SENSOR_STATUS_ACCURACY_HIGH;

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
    if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_MAGV_X), &absinfo_x) &&
        !ioctl(data_fd, EVIOCGABS(EVENT_TYPE_MAGV_Y), &absinfo_y) &&
        !ioctl(data_fd, EVIOCGABS(EVENT_TYPE_MAGV_Z), &absinfo_z)) {
        value = absinfo_x.value;
        mPendingEvents[MagneticField].magnetic.x = value;// * CONVERT_GYRO_X;
        value = absinfo_x.value;
        mPendingEvents[MagneticField].magnetic.y = value;// * CONVERT_GYRO_Y;
        value = absinfo_x.value;
        mPendingEvents[MagneticField].magnetic.z = value;// * CONVERT_GYRO_Z;
        mHasPendingEvent = true;
    }
    return 0;
}

int MagneticSensor::enable(int32_t handle, int en) {
     int what = -1;
	ALOGE("ENABLE MAGNETIC SENSOR");
    switch (handle) {
        case ID_M: what = MagneticField; break;
        case ID_O: what = Orientation;   break;
    }
	   if (uint32_t(what) >= numSensors)
        return -EINVAL;

    int newState  = en ? 1 : 0;
    int err = 0;

    if ((uint32_t(newState)<<what) != (mEnabled & (1<<what))) {

        uint32_t sensor_type;

        switch (what) {
            case MagneticField: sensor_type = SENSOR_TYPE_MAGNETIC_FIELD; 
								ALOGE("ENABLE MAGSENSOR - SWITCHED");
								break;
        }
        short flags = newState;
  /*      if (en){
            err = akm_enable_sensor(sensor_type);
        }else{
            err = akm_disable_sensor(sensor_type);
        }*/

        err = sspEnable(LOGTAG, SSP_MAG, en);
        setInitialState();

        ALOGE_IF(err, "Could not change sensor state (%s)", strerror(-err));
        if (!err) {
            mEnabled &= ~(1<<what);
            mEnabled |= (uint32_t(flags)<<what);
        }
    }
    return err;
}


bool MagneticSensor::hasPendingEvents() const {
    /* FIXME probably here should be returning mEnabled but instead
      mHasPendingEvents. It does not work, so we cheat.*/
    //ALOGD("MagneticSensor::~hasPendingEvents %d", mHasPendingEvent ? 1 : 0 );
    return mHasPendingEvent;
}


int MagneticSensor::setDelay(int32_t handle, int64_t ns)
{
    int fd;

    if (ns < 10000000) {
        ns = 10000000; // Minimum on stock
    }
	ALOGW("MAGNETIC_SENSOR: Set Delay");
    strcpy(&input_sysfs_path[input_sysfs_path_len], "mag_poll_delay");
    fd = open(input_sysfs_path, O_RDWR);
    if (fd >= 0) {
        char buf[80];
        sprintf(buf, "%lld", ns / 10000000 * 10); // Some flooring to match stock value
        write(fd, buf, strlen(buf)+1);
        close(fd);
        return 0;
    }
    return -1;
}


int MagneticSensor::readEvents(sensors_event_t* data, int count)
{
	ALOGW("MAGNETIC_SENSOR: Read Events");
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
        if (type == EV_REL || type == EV_ABS) {
            float value = event->value;
			/* ACTUAL DATA PROCESSING GOES HERE */
			ALOGE("MAGNETIC_SENSOR: value DETECTED!");
			switch (event->value) {
				case REL_RX:
				ALOGE("MAGNETIC_SENSOR:REL_RX");
					mPendingEvents[MagneticField].magnetic.x = value;
					break;
				case REL_RY:
				ALOGE("MAGNETIC_SENSOR:REL_RY");
					mPendingEvents[MagneticField].magnetic.y = value;
					break;
				case REL_RZ:
				ALOGE("MAGNETIC_SENSOR: REL_RZ");			
					mPendingEvents[MagneticField].magnetic.z = value;
					break;
				case REL_HWHEEL:
				ALOGE("MAGNETIC_SENSOR: HWHEEL REPORTED");
					//mPendingEvent[MagneticField].magnetic.z = value;
					break;
				default:
					ALOGE("DEFAULT");
			}
        } else if (type == EV_SYN) {
            mPendingEvents[MagneticField].timestamp = timevalToNano(event->time);
            if (mEnabled) {
                *data++ = mPendingEvent;
                count--;
                numEventReceived++;
            }
        } else {
            ALOGE("%s: unknown event (type=%d, code=%d)", LOGTAG,
                    type, event->code);
        }

        mInputReader.next();
    }
    return numEventReceived++;

}
