/*
 * Copyright (c) 2019 Future Electronics
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef TYPED_ENUM_DRIVER_H_
#define TYPED_ENUM_DRIVER_H_


template <typename IntType> class TypedEnum
{
public:
    TypedEnum(IntType init_val) : _val(init_val) {}

    operator IntType() const { return _val; };

protected:
    IntType _val;
};

#endif // TYPED_ENUM_DRIVER_H_
