extern "C"{
    #include "MD5Hashing.h"
};

#include <SPI.h>
#include <PN532_SPI.h>
#include <emulatetag.h>
#include <NdefMessage.h>

PN532_SPI pn532spi(SPI, 10);
EmulateTag nfc(pn532spi);

uint8_t ndefBuf[50];
uint8_t uid[3] = { 0x12, 0x34, 0x56 };

/* Tag Key : 010ff43bca12erh6 */
unsigned char tagk[]  = "010ff43bca12erh6";
/* Tag ID : 2537*/
String tagID = "2537";
/*Tag Info = Tag Key + Tag ID, len = 20*/
unsigned char tagInfo[] = "010ff43bca12erh62537";
int counter = 0;

void setup()
{
    Serial.begin( 9600 );
    while (!Serial) { ; /* wait for serial port to connect. Needed for Leonardo only */ }
    Serial.println("------- Data Verifier --------");
    
    randomSeed( analogRead(0) );
    
    // uid must be 3 bytes!
    nfc.setUid( uid );
    nfc.init();
}

void loop()
{
    nfc.emulate();
    
    if(nfc.writeOccured())
    {
        uint8_t* tag_buf;
        uint16_t length;
        
        counter++;
        if( counter > 99 ) counter = 99;
  
        nfc.getContent( &tag_buf , &length );
        NdefMessage msg = NdefMessage( tag_buf , length );
        
        for( int i = 0 ; i < msg.getRecordCount() ; i++ )
        {
            Serial.print("\n");
            NdefRecord record = msg.getRecord(i);
            
            int plen = record.getPayloadLength();
            byte payload[ plen ];

            // NFC get data
            record.getPayload( payload );
            
            Serial.print("Got: ");
            String pString = "";
            unsigned char data[ (plen - 1) ];
            char *toPrint;
            
            /* Preparaing data to use & print it out. */
            for ( int c = 0 ; c < plen ; c++ )
            {
                pString += (char) payload[ c ];
                
                if( c == 0 )  continue;
                
                toPrint = (char*) &pString[ c ];
                
                data[ (c - 1) ] = static_cast<unsigned char>( toPrint[0] );
                Serial.print( toPrint[0] );
            }
            Serial.print("\n");
            
            String md5 = "", tk ="";
            Serial.print("Tk:  ");
            toPrint = "";
            
            for ( int c = 0 ; c < 16 ; c++ )
            {
                toPrint = (char*) &tagk[ c ];
                Serial.print( toPrint[ 0 ] );
            }
            // (tk_info | data)
            unsigned char datatk[ 41 ];
            
            for(int x = 0 ; x < 38 ; x++)
            {
                if( x < 20 )  datatk[ x ] = tagInfo[ x ];
                else          datatk[ x ] = data[ x - 20 ];
            }
            
            for(int i = 0 ; i < 3 ; i++)
            {
              datatk[ 38 + i ] = data[ 18 + i ] - '0';
            }

            Serial.print("\nTo Hash: ");
            for( i = 0 ; i < 41 ; i++ )
            {
              Serial.print(datatk[i]);
              Serial.print(" ");
            }

            // Generating the Tag Signature
            generateMD5(datatk , 41);
            for( int x = 0 ; x <= 15 ; x++ )  md5 += printMD5( (unsigned char *) &MD5Digest[ x ] , 1 , x );

            // Updating the Tag key and Tag info
            generateMD5(tagk , 16);
            for( int x = 0 ; x <= 15 ; x++ )
            {
                tagk[ x ]     = printMD5( (unsigned char *) &MD5Digest[ x ] , 1 , x );
                tagInfo[ x ]  = tagk[ x ];
            }

            Serial.print("\nComplete Data: ");

            for( i = 0 ; i < 4 ; i++ )  md5 += tagID[ i ];

            Serial.println( md5 );
            
            NdefMessage M = NdefMessage();
            M.addTextRecord( md5 );
            M.encode(ndefBuf);
            nfc.setNdefFile(ndefBuf, M.getEncodedSize());
        }
    }
    delay(1000);
}
