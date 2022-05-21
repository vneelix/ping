/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   argv_handler.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vneelix <vneelix@student.21-school.ru>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/03/15 00:19:44 by vneelix           #+#    #+#             */
/*   Updated: 2022/05/22 00:02:35 by vneelix          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ping.h"

static int invalid_value_callback(const char *arg, const char *value, int return_code) {
	fprintf(stderr, "ft_ping: %s invalid value: %s\n", arg, value);
	return (return_code);
}

static int out_of_range_callback(const char *arg, const char *value,
									long int min, long int max, int return_code) {
	fprintf(stderr, "ft_ping: %s invalid value: %s: "
					"out of range: %ld <= value <= %ld\n", arg, value, min, max);
	return (return_code);
}

int	argv_handler(int argc, char *argv[], struct preset *preset) {
	const char	*address = NULL;

	for (int i = 1; i != argc; i++) {
		if (argv[i][0] != '-') {
			address = argv[i];
			continue;
		}
		if (ft_strlen(argv[i]) != 2) {
			fprintf(stderr, "ft_ping: invalid option: %s\n", argv[i]);
			return (-1);
		}

		switch (argv[i][1]) {
			case 'h':
				break;
			case 'v':
				break;
			case '4':
				preset->domain = AF_INET;
				break;
			case '6':
				preset->domain = AF_INET6;	
				break;
			case 't':
				if (i + 1 == argc || !is_number(argv[i + 1]))
					return (invalid_value_callback(argv[i], i + 1 == argc ? "(null)" : argv[i + 1], -1));
				int ttl = ft_atoi(argv[i + 1]);
				if (ttl < 1 || ttl > 255)
					return (out_of_range_callback(argv[i], argv[i + 1], 1, 255, -1));
				i++;
				preset->flag.ttl = 1;
				preset->ttl = (uint8_t)ttl;
				break;
			case 'c':
				if (i + 1 == argc || !is_number(argv[i + 1]))
					return (invalid_value_callback(argv[i], i + 1 == argc ? "(null)" : argv[i + 1], -1));
				long int c = ft_atol(argv[i + 1]);
				if (c < 1 || c > INT64_MAX)
					return (out_of_range_callback(argv[i], argv[i + 1], 1, INT64_MAX, -1));
				i++;
				preset->flag.infinite_attempts = 0;
				preset->attempts_count = c;
				break;
			case 's':
				if (i + 1 == argc || !is_number(argv[i + 1]))
					return (invalid_value_callback(argv[i], i + 1 == argc ? "(null)" : argv[i + 1], -1));
				int s = ft_atol(argv[i + 1]);
				if (s < 0 || c > UINT16_MAX)
					return (out_of_range_callback(argv[i], argv[i + 1], 0, 64000, -1));
				i++;
				preset->payload_len = s;
				break;
			case 'i':
				if (i + 1 == argc || !is_float(argv[i + 1]))
					return (invalid_value_callback(argv[i], i + 1 == argc ? "(null)" : argv[i + 1], -1));
				preset->interval = ft_atof(argv[i + 1]) * 1000000.;
				if (s < 0 || c > INT64_MAX)
					return (out_of_range_callback(argv[i], argv[i + 1], 0, INT64_MAX, -1));
				i++;
				break;
			default:
				fprintf(stderr, "ft_ping: invalid option: %s\n", argv[i]);
				return (-1);
				break;
		}
	}
	if (address == NULL) {
		fprintf(stderr, "ft_ping: usage error: destination address required\n");
		return (-1);
	}
	if (!dns_lookup(address, preset->domain, &preset->ai)) {
		fprintf(stderr, "ft_ping: %s: name or service not known\n", address);
		return (-1);
	}
	return (0);
}
