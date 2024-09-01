/*
 * pll1_check_stb7100.c
 * 
 * nit 08.11.2010
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */
 
main(int argc, char * argv[])
{
 int ndiv=0, mdiv=0;
 float frequ = 0, frequ_check=1;
 unsigned long ndiv_mdiv=0;

 if(argc != 2)
 {
	printf("usage: %s frequenz\n", argv[0]);
	exit(1);
 }

 frequ=atoi(argv[1]);

 frequ=frequ / 2 / 27;

 while(frequ_check != ndiv && mdiv<=255 && ndiv<=255)
 {
  mdiv++;
  frequ_check = frequ * (mdiv *2);
  ndiv = frequ_check;
 }

 if(mdiv<0 || mdiv>255)	
 {
	printf("mdiv not correct (%d)\n", mdiv);
	printf("frequenz can't be used\n");
	exit(1);
 }
 if(ndiv<3 || ndiv>255)	
 {
	printf("ndiv not correct (%d)\n", ndiv);
	printf("frequenz can't be used\n");
	exit(1);
 }
 
 printf("frequ=%s\n",argv[1]);
 printf("ndiv=%d (%x)\n",ndiv, ndiv);
 printf("mdiv=%d (%x)\n",mdiv, mdiv);
 ndiv_mdiv = mdiv;
 ndiv_mdiv += (ndiv << 8);
 printf("pll1_ndiv_mdiv for module=%ld (%lx)\n",ndiv_mdiv, ndiv_mdiv);
 printf("frequenz is ok\n");
}
