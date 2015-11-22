/*
 * sensord
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef _AUTO_ROTATION_ALG_H_
#define _AUTO_ROTATION_ALG_H_

class auto_rotation_alg
{
public:
	auto_rotation_alg();
	virtual ~auto_rotation_alg();

	virtual bool open(void);
	virtual bool close(void);
	virtual bool start(void);
	virtual bool stop(void);
	virtual bool get_rotation(float acc[3], unsigned long long timestamp, int prev_rotation, int &rotation) = 0;
};
#endif /* _AUTO_ROTATION_ALG_H_ */
