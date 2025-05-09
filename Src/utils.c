/*
 * utils.c
 *
 *  Created on: May 4, 2025
 *      Author: victo
 */
int get_ramdom(int min, int max) {
    // tu implementación aquí
    return (rand() % (max - min + 1)) + min;
}

