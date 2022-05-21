/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   is_number.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vneelix <vneelix@student.21-school.ru>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/06/19 17:28:56 by vneelix           #+#    #+#             */
/*   Updated: 2022/05/21 23:59:06 by vneelix          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "minilib.h"

int	is_number(const char *str)
{
	if (*str == '-' || *str == '+')
		str++;
	while (*str != '\0') {
		if (*str < '0' || *str > '9')
			return (0);
		str++;
	}
	return (1);
}

int	is_float(const char *str)
{
	int point = 0;
	if (*str == '-' || *str == '+')
		str++;
	while (*str != '\0') {
		if (*str == '.' && point == 0) {
			point = 1;
			str++;
		}
		if (*str < '0' || *str > '9')
			return (0);
		str++;
	}
	return (1);
}
