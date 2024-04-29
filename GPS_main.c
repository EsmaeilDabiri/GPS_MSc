#define F_CPU 3333333
#define USART1_BAUD_RATE(BAUD_RATE) ((float)(F_CPU * 64 / (16 * (float)BAUD_RATE)) + 0.5)

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define AOI_lat_north 100 
#define AOI_long_east 50 

#include <math.h>

#define R 6371.0 // Earth radius in kilometers
static void USART1_sendByte(uint8_t c)
{
    while (!(USART1.STATUS & USART_DREIF_bm));
    USART1.TXDATAL = c;
}
uint8_t USART1_receiveByte() {
    // Wait for data to be received
   while (!(USART1.STATUS & USART_RXCIF_bm));
    return USART1.RXDATAL;
}

static void USART1_init(void)
{
    PORTC.DIRSET = PIN0_bm;
    PORTC.DIRCLR = PIN1_bm;
    
    USART1.BAUD = (uint16_t)USART1_BAUD_RATE(9600); 
    
    USART1.CTRLB |= USART_TXEN_bm | USART_RXEN_bm;  
   //USART1_CTRLC |= USART_CHSIZE_8BIT_gc; 
}
double calculateCoverageRadius(double fov_degrees, double altitude_km) {
    double half_fov_radians = fov_degrees * (M_PI / 180.0) / 2.0; // Convert FOV from degrees to radians and divide by 2
    double coverage_radius_km = altitude_km * tan(half_fov_radians); // Calculate coverage radius

    return coverage_radius_km;
}
double degreesToRadians(double degrees) {
    return degrees * M_PI / 180.0;
}
double haversine(double lat1, double lon1, double lat2, double lon2) {
    double dlat, dlon, a, c, distance;
    
    // Convert latitude and longitude from degrees to radians
    lat1 = degreesToRadians(lat1);
    lon1 = degreesToRadians(lon1);
    lat2 = degreesToRadians(lat2);
    lon2 = degreesToRadians(lon2);
    
    // Differences in latitudes and longitudes
    dlat = lat2 - lat1;
    dlon = lon2 - lon1;
    
    // Haversine formula
    a = pow(sin(dlat / 2), 2) + cos(lat1) * cos(lat2) * pow(sin(dlon / 2), 2);
    c = 2 * atan2(sqrt(a), sqrt(1 - a));
    distance = R * c;
    
    return distance;
}

#define FOV_degrees 4
#define ALTITUTE_Km 2000
#define lat_AOI_north_degrees 55
#define long_AOI_east_degrees 15

char sentence[100];
char time[20] ,status, Latitude[20],north_south ,Longitude[20] ,east_west ,Speed_over_the_ground_in_knots[20], Track_angle_in_degrees[20] , Date[20] ,Magnetic_Variation[20], Magnetic_Variation_Direction[20] ;
int x=1;
double my_lat, my_long , coverage_radius, distance_radius ;
uint64_t data;

char *portion;
int index = 0;
       int i=0;

int main(void)
{
    PORTA_DIRCLR = PIN2_bm; // receive telemetry for saving to memory
    PORTA_DIRSET = PIN0_bm; // camera
    PORTA_OUTCLR = PIN0_bm; //camera off
    
    PORTB_DIRSET = PIN3_bm;
PORTB_OUTSET = PIN3_bm;
    USART1_init();
    
    while (1) 
    {
     x=1;
         switch (x){
                    case 1:   
                        //Receive data from USART
                       data = USART1_receiveByte();
                        index = 0;
                        // Buffer the complete sentence
                        while(data != '\n') {
                            sentence[index] = USART1_receiveByte();
                            index++;
                        } 
                        for (index=0 ; index<=5 ; index++){
                           sentence[index] = USART1_receiveByte(); 
                        }
                        
                        //PORTB_OUTCLR = PIN3_bm; 
                        // Null-terminate the string
                        sentence[6] = '\n'; //here,the full line is ready
                        // if it is GPRMC sentence
                        if (strncmp(sentence, "$GPRMC", 6) == 0) 
                         x=2;
                    break;
                    
                    case 2:   
                         // Process the GPRMC sentence
                        portion = strtok(sentence ,","); //portion = $GPRMC
                        i=0;
                            while (portion != NULL) {
                            portion = strtok(NULL, ",");
                            i++;
                            switch (i){
                                case 1:   
                                       time[20] = portion ;
                                        break;
                                case 2:   
                                        status = portion ; //A=active , V=void
                                       if(status=='V')
                                       {
                                           portion = NULL; //get out of the loop
                                           x=1;
                                       }
                                        break;
                                case 3:   
                                        Latitude[20]  = portion ;
                                        my_lat = atof(Latitude);
                                        break;
                                case 4:   
                                        north_south = portion ;
                                        if(north_south == 'S')
                                           my_lat = 360-my_lat; 
                                        break;        
                                case 5:   
                                        Longitude[20]  = portion ;
                                        my_long = atof(Longitude);
                                        break;       
                                case 6:   
                                        east_west = portion ;
                                        if(north_south == 'W')
                                           my_long = 360-my_long;
                                        break;        
                                case 7:   
                                        Speed_over_the_ground_in_knots[20] = portion ;
                                        break;        
                                case 8:   
                                        Track_angle_in_degrees[20] = portion ;
                                        break;        
                                case 9:   
                                        Date[20]  = portion ;
                                        break;       
                                case 10:   
                                        Magnetic_Variation[20] = portion ;
                                        break;        
                                case 11:   
                                        Magnetic_Variation_Direction[20] = portion ;
                                        break;        
                                }}

                                break;
                 
                    
                    case 3:   
                            coverage_radius = calculateCoverageRadius(FOV_degrees,ALTITUTE_Km);
                            distance_radius = haversine(lat_AOI_north_degrees, long_AOI_east_degrees, my_lat, my_long);
                            if (distance_radius <= coverage_radius) //area is in range
                                x=4;
                            else if (PORTA_IN & PIN2_bm)
                            {
                                x=5;
                                PORTA_OUTCLR = PIN0_bm; //camera off
                            }
                            else 
                            {
                                x=1;
                                PORTA_OUTCLR = PIN0_bm; //camera off
                            }
                    break;
                    case 4:   
                           PORTA_OUTSET = PIN0_bm; //camera ON   
                           if (PORTA_IN & PIN2_bm)
                            {
                                x=5;
                            }
                            else 
                                x=1;
                    break;
                    case 5:   
                           //save to memory code   
                        
                        x=1;
                    break;
        
        
        
        
        
        
        
        
        
        
        
        }

           
               
            
        
         
        
        

         }}