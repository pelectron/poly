/*
 * properties.hpp
 *
 *  Created on: Jan 5, 2024
 *      Author: pelec
 */

#ifndef INC_PROPERTIES_HPP_
#define INC_PROPERTIES_HPP_

#include "poly/interface.hpp"



#define METHODS(...) poly::type_list<__VA_ARGS__>
#define PROPERTIES(...) poly::type_list<__VA_ARGS__>

#endif /* INC_PROPERTIES_HPP_ */
