#include <GL/glut.h>

#include <windows.h> //- MessageBoxA
#include <stdio.h>
#include <stdlib.h>

#include "defines.h" //- Definições

/**
 * Comente esta linha caso deseje desativar o debug durante a compilação
 * (((WARNING))): Esta opção ativa deixa o jogo muito lentro na hora de desenhar
 */
//#define DEBUG (0x01 | 0x02)

/**
 * Estas definições servem para indicar se o programa deverá rodar usando
 * swap de buffers ou flush diretamente, comente a linha abaixo caso deseje
 * que o programa trabalhe com flush() para rodar em máquinas mais antigas sem
 * muito poder de processamento.
 */
#define USE_DOUBLE

/**
 *  Não altere estas linhas abaixo
 */
#ifdef USE_DOUBLE
	#define GLUT_TIPO GLUT_DOUBLE
	#define glAplica glutSwapBuffers()
#else
	#define GLUT_TIPO GLUT_SINGLE
	#define glAplica glFlush()
#endif
//-----------------------------------


void TimerAnimaGhost(int id);

/**
 * Matriz do mapa do jogo onde cada elemento indica
 * o tipo de objeto que esta na posição
 */
tpBlock MAP[NUM_BLOCKS_VER][NUM_BLOCKS_HOR];

/**
 * Este vetor é semelhante ao anterior porém ele será utilizado
 * para restaurar as posições dos GHOSTS e do HEROI quando o 
 * mesmo colidir
 */
tpBlock ReloadMAP[NUM_BLOCKS_VER][NUM_BLOCKS_HOR];

/**
 * Estas variáveis guardarão as posições correntes do pacman
 */
GLint heroi_x, heroi_y;

/**
 * Número de LUNCH carregados no mapa
 */
GLint num_lunch = 0;

/**
 * Número de vidas
 */
GLint num_lifes = NUM_LIFES;

/**
 * Se o tempo de animação dos GHOSTs devem continuar
 */
GLint continue_timer = 0;

/**
 * Esta função é responsável por carregar o arquivo de definição
 * de mapa. Onde cada byte do arquivo define o tipo de elemento
 * no mapa e sua disposição na matriz e consequentemente na tela
 */
void IniciaMapa() {
     FILE *f;
     int x = 0, y = 0, achou_heroi=0;
	 char c;
     
     
     //- Abre o arquivo com texto e somente-leitura
     if (!(f = fopen(".\\mapa.txt", "r+"))) {
	     MessageBoxA(NULL, "Erro ao carregar mapa.", "Erro", 0);
	     exit(0);
     }

	/**
	 * Lê um novo caracter e insere na matriz do mapa seguindo
	 * tipo que ele representa
	 */    
    while (!feof(f) && (c = fgetc(f))) {

		//- Verifica se está além da dimensão da matriz do mapa
        if (x == NUM_BLOCKS_HOR) {
            x = 0;
            y++;
        }

        if (y == NUM_BLOCKS_VER) {
            /**
             * Se tive no final é porque acabou de preencher o mapa e chegou
             * no final do arquivo
             */
            if (feof(f)) {
                fclose(f);
                return;
            } else {    
                fclose(f);
    			MessageBoxA(NULL, "Mapa em formato inválido.", "Erro", 0);
    			exit(0);
            }
        }
        
        /**
		 * Ingora todos os caracteres de nova linha, deslocando apenas
         * para a próxima linha da matriz
		 *(No padrão ANSI o caracter ASCii 10 representa uma nova linha)
		 */
        if (c != 0x0a) {
            MAP[y][x].iType = (c - '0');
            
            switch (MAP[y][x].iType) {
                case BLOCK_HEROI :
                    if (!achou_heroi) {

                        //- Grava a posição inicial do heroi
                        heroi_x = x;
                        heroi_y = y;
                        
#ifdef DEBUG
                        if (DEBUG & 0x01) {
                            printf ("Heroi encontrado:\n");
                            printf (" X: %d - Y: %d\n", heroi_x, heroi_y);
                        }
#endif
                        achou_heroi = 1;
                    } else {
                        //- Só permite um pacman no jogo
                        MessageBoxA(0, "O jogo só permite 1 único heroi.\nEdite o arquivo de mapa.", "Erro", 0);
                        exit(0);
                    }
                    
                    ReloadMAP[y][x].iType = BLOCK_HEROI;

                    break;

                case BLOCK_LUNCH :
					//- Seta o flag to block indicando que é LUNCH
                    MAP[y][x].iFlags |= BLOCK_RIDER_LUNCH;

                    num_lunch++;
                    
                    break;

                case BLOCK_GHOST :
					//- Seta o flag to block indicando que é LUNCH
                    MAP[y][x].iFlags |= BLOCK_RIDER_GHOST;
	                ReloadMAP[y][x].iType = BLOCK_GHOST;
	                ReloadMAP[y][x].iFlags |= BLOCK_RIDER_GHOST;

                    break;
                    
            }
#ifdef DEBUG
            if (DEBUG & 0x04)
                printf (" [%d, %d] Heroi (%d - %d) [%c]\n", x, y, heroi_x, heroi_y, c);
#endif
            x++;
        }
    }

	//- Fecha o arquivo =)
	fclose(f);
}


void Inicializa() {
    IniciaMapa(); //- Carrega o mapa

	/**
	 * Inicia definindo as propriedades
	 */
	glutInitDisplayMode(GLUT_TIPO | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutCreateWindow(PROGNAME" - "VERSION);
	glutFullScreen(); //- Tela cheia

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    gluOrtho2D(X_MIN, X_MAX, Y_MIN, Y_MAX);    

#ifdef DEBUG
    if (DEBUG & 0x01) {
        printf ("Window Properties\n");
        printf (" - Width: %dpx\n", WINDOW_WIDTH);
        printf (" - Height: %dpx\n", WINDOW_HEIGHT);
        printf (" - X Min: %d | X Max: %d\n", X_MIN, X_MAX);
        printf (" - Y Min: %d | Y Max: %d\n", Y_MIN, Y_MAX);
    }
#endif
}

/**
 * Esta função desenha o bloco b na posição especificada pelos parametros i e j
 */
void DesenhaBlock(int i, int j) {
    
    tpBlock b = MAP[Y_MAX-j][i-X_MIN]; //- Obtém o bloco da posição definida
    
	glBegin(GL_QUADS);
	{
		//- Verifica se é um bloco válido
	    if (
	        b.iType == BLOCK_RIDER || b.iType == BLOCK_GHOST || b.iType == BLOCK_HEROI ||
			b.iType == BLOCK_WALL  || b.iType == BLOCK_LUNCH
	    ) {
			/**
			 * Se for chão então verifica se possui algum flag ativo para este
			 * bloco e muda o tipo do bloco se necessário
			 */
	        if (b.iType == BLOCK_RIDER) {
	            //if (b.iFlags & BLOCK_RIDER_GHOST) b.iType = BLOCK_GHOST;
	            if (b.iFlags & BLOCK_RIDER_LUNCH) b.iType = BLOCK_LUNCH;
	        }
	
	        int k, l;
	        float x2, y2;
	
			//- Desenha o bloco pixel por pixel de acordo com o tipo definido
	        for (k = 0; k < LEN_BLOCK; k++) {
	            for (l = 0; l < LEN_BLOCK; l++) {

					y2 = j - (k * LEN_POINT);
					x2 = i + (l * LEN_POINT);

					//- Especifica a cor pela matriz do bitmap
	                glColor3fv(Pictures[b.iType][k][l]);
	
	                glVertex2f(x2, y2);
	                glVertex2f(x2 + LEN_POINT, y2);
	                glVertex2f(x2 + LEN_POINT, y2 - LEN_POINT);
	                glVertex2f(x2, y2 - LEN_POINT);
	            }
	        }
/**
	        switch (b.iType) {
	            case BLOCK_GHOST :
	                b.iFlags |= BLOCK_RIDER_GHOST;
	                break;
	            case BLOCK_LUNCH :
	                b.iFlags |= BLOCK_RIDER_LUNCH;
	                break;
	        }
**/	
	    } else {
	        glColor3f(1.0f, 0.0f,0.0f);
	        glVertex2f(i, j-1);
	        glVertex2f(i+1, j-1);
	        glVertex2f(i+1, j);
	        glVertex2f(i, j);
	    }
	} glEnd();
}


void Desenha() {

	//- Limpa a tela
	glClearColor(0.9f, 0.9f, 0.9f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

    GLint x, y;

    for (x = X_MIN; x < X_MAX; x++)
        for (y = Y_MAX; y > Y_MIN; y--) {
#ifdef DEBUG
    if (DEBUG & 0x08) {
            printf ("MAP[%d][%d].iType = %d\n", x-X_MIN, Y_MAX-y, MAP[x-X_MIN][Y_MAX-y].iType);
    }
#endif
			DesenhaBlock(x, y);
        }
	
	glAplica;

}


void TorturaHeroi() {
	//- Para a animação dos GHOSTS
	continue_timer = 0;
	
	//- Decrementa a vida
	num_lifes--;
	
	
	char msg[128];
	GLint x, y;

	if (!num_lifes) {
		MessageBoxA(0, "Ihh... Perdeu maneh!!! =)", ":(", 0);
		exit(0);
	}
	
	sprintf (msg, "SÓ LHE RESTAM %d VIDAS\nENJOY", num_lifes);
	MessageBoxA(0, msg, "Continue tentando :\\", 0);
	
	//- Restaura a posição do HEROI e dos GHOSTs
	for (x = 0; x < NUM_BLOCKS_HOR; x++)
		for (y = 0; y < NUM_BLOCKS_VER; y++) {

			if ((MAP[y][x].iType == BLOCK_GHOST) || (MAP[y][x].iType == BLOCK_HEROI)) {
				MAP[y][x].iType = BLOCK_RIDER;
				MAP[y][x].iType &= ~(BLOCK_RIDER_GHOST | BLOCK_RIDER_HEROI);
			}

			switch (ReloadMAP[y][x].iType) {
				case BLOCK_GHOST :
					MAP[y][x].iType = BLOCK_GHOST;
					break;
				case BLOCK_HEROI :
					MAP[y][x].iType = BLOCK_HEROI;

					//- Grava a posição atual do HEROI
					heroi_x = x;
					heroi_y = y;
					break;
			}
			
		}
		
		
	Desenha();
	glutPostRedisplay();
	continue_timer = 1;
	glutTimerFunc(500, TimerAnimaGhost, 0);
	
	
}



void TimerAnimaGhost(int id) {

    GLint x, y;         //- Para percorrer a matriz
    GLint dif_x, dif_y; //- Diferença das direções. Para percorrer o melhor caminho
    GLint d;            //- Direção em que o fantasma deverá percorrer
    GLint s=-1;         //- Sentido em que o fantasma deverá ir (-1 - Não mover, 0 - Vertical, 1 - Horizontal)

    //- Marca todos os blocos como não processados
    for (x = 0; x < NUM_BLOCKS_HOR; x++)
        for (y = 0; y < NUM_BLOCKS_VER; y++)
            MAP[y][x].iFlags &= ~BLOCK_CRTL_CHANGED;

    //- Procura todos os fantasmas que existirem
    for (x = 0; x < NUM_BLOCKS_HOR; x++) {
        for (y = 0; y < NUM_BLOCKS_VER; y++) {

            //- Obtem o block da posição atual e verifica se é um fantasma e se ainda
            //- não foi modificado
            if ((MAP[y][x].iType == BLOCK_GHOST) && ((MAP[y][x].iFlags & BLOCK_CRTL_CHANGED) == 0)) {
                dif_x = abs(x - heroi_x);
                dif_y = abs(y - heroi_y);

                //- Movimento horizontal tem maior prioridade ?
                if (dif_x > dif_y)
                {
                    //- Calcula a direção em que deve andar (-1: Esquerda, 1: Direita)
                    d = (x - heroi_x > 0 ? -1 : 1);

                    //- Verifica se consegue mover (não tem nenhum fantasma e nenhuma parece no próximo bloco)
                    if ((x+d > 0) && (x+d < NUM_BLOCKS_HOR) && (MAP[y][x + d].iType != BLOCK_WALL) && (MAP[y][x + d].iType != BLOCK_GHOST))
                    {
                        s = 1; //- Marca como movimento horizontal
                    }
                    else //- Força movimento vertical
                    {
                        //- Calcula a direção em que deve andar (-1: Cima, 1: Baixo)
                        d = (y - heroi_y > 0 ? -1 : 1);

                        //- Verifica se consegue mover
                        if ((y+d > 0) && (y+d < NUM_BLOCKS_VER) && (MAP[y + d][x].iType != BLOCK_WALL) && (MAP[y + d][x].iType != BLOCK_GHOST))
                            s = 0; //- Marca como movimento vertical
                    }
                }

                else //- Movimento vertical tem maior prioridade ?
                {
                    //- Calcula a direção em que deve andar (-1: Cima, 1: Baixo)
                    d = (y - heroi_y > 0 ? -1 : 1);
                    if ((y+d > 0) && (y+d < NUM_BLOCKS_VER) && (MAP[y + d][x].iType != BLOCK_WALL) && (MAP[y + d][x].iType != BLOCK_GHOST))
                    {
                        s=0;
                    }
                    else
                    {
                        //- Calcula a direção em que deve andar (-1 - Esquerda, 1 - Direita)
                        d = (x - heroi_x > 0 ? -1 : 1);                        
                        if ((x+d > 0) && (x+d < NUM_BLOCKS_HOR) && (MAP[y][x + d].iType != BLOCK_WALL) && (MAP[y][x + d].iType != BLOCK_GHOST))
                            s=1;
                    }
                }

                //- Se consegue mover-se
                if (s != -1) {
                    //- Transforma o bloco atual em RIDER
                    MAP[y][x].iType = BLOCK_RIDER;
                    MAP[y][x].iFlags &= ~BLOCK_RIDER_GHOST;

                    DesenhaBlock(X_MIN + x, Y_MAX - y);

                    /**
                     * Marca e desenha o próximo bloco como alterado para
                     * evitar que seja processado novamente nesta rodada
                     */
                    if (s) {
                        MAP[y][x+d].iType = BLOCK_GHOST;
                        MAP[y][x+d].iFlags |= BLOCK_CRTL_CHANGED;
                        MAP[y][x+d].iFlags |= BLOCK_RIDER_GHOST;

                        DesenhaBlock(X_MIN + x + d, Y_MAX - y);
                        
                        if ((x+d == heroi_x) && (heroi_y == y))
                        	TorturaHeroi();

                    } else {
                        MAP[y+d][x].iType = BLOCK_GHOST;
                        MAP[y+d][x].iFlags |= BLOCK_CRTL_CHANGED;
                        MAP[y+d][x].iFlags |= BLOCK_RIDER_GHOST;                        

                        DesenhaBlock(X_MIN + x, Y_MAX - y - d);

                        if ((heroi_x == x) && (heroi_y == (y+d)))
                        	TorturaHeroi();

                    }
                    
                    
                    glAplica;
                }
                
            }
        }
    }

	if (continue_timer) 
    	glutTimerFunc(500, TimerAnimaGhost, 0);
}


/**
 * Esta função verifica o movimento do HEROI :)
 * e trata outras teclas de atalho
 *   F2  - Reinicia o jogo
 *   F12 - Alterna entre janela e tela cheia
 */
void TeclasEspeciais(int tecla, int x, int y)
{
    GLint mudou = 0;
    GLint old_heroi_x = heroi_x;
    GLint old_heroi_y = heroi_y;
    
    
    
    switch (tecla) {
        case GLUT_KEY_LEFT :
            if ((heroi_x > 0) && (MAP[heroi_y][heroi_x - 1].iType != BLOCK_WALL)) {
                heroi_x--;
                mudou = 1;
            }
            break;
        case GLUT_KEY_RIGHT :
            if ((heroi_x < (NUM_BLOCKS_HOR - 1)) && (MAP[heroi_y][heroi_x + 1].iType != BLOCK_WALL)) {
                heroi_x++;
                mudou = 1;
            }
            break;
        case GLUT_KEY_UP :
            if ((heroi_y > 0) && (MAP[heroi_y - 1][heroi_x].iType != BLOCK_WALL)) {
                heroi_y--;
                mudou = 1;
            }
            break;
        case GLUT_KEY_DOWN :
            if ((heroi_y < (NUM_BLOCKS_VER - 1)) && (MAP[heroi_y + 1][heroi_x].iType != BLOCK_WALL)) {
                heroi_y++;
                mudou = 1;
            }
            break;
        case GLUT_KEY_F3 :
            TimerAnimaGhost(0);
            break;
    }
    
    if (mudou) {
#ifdef DEBUG
	    if (DEBUG & 0x02) {
			printf ("Posicao no vetor:\n");
	        printf ("  X: %d -> %d\n", old_heroi_x, heroi_x);
	        printf ("  Y: %d -> %d\n", old_heroi_y, heroi_y);

			printf ("Posicao na tela:\n");
	        printf ("  X: %d -> %d\n", X_MIN + old_heroi_x, X_MIN + heroi_x);
	        printf ("  Y: %d -> %d\n", Y_MAX + old_heroi_y, Y_MAX + heroi_y);

	    }
#endif
        //- Inverte as posições no vetor
        MAP[heroi_y][heroi_x].iType = BLOCK_HEROI;
        MAP[old_heroi_y][old_heroi_x].iType = BLOCK_RIDER;

        /**
         * Poderiamos tentar animar a troca para não deixar tão sólida
         */
        DesenhaBlock(
			X_MIN + heroi_x,
			Y_MAX - heroi_y
		);

        DesenhaBlock(
			X_MIN + old_heroi_x,
			Y_MAX - old_heroi_y
		);

		glAplica;


		/**
		 *  Verifica se ouve colisão com algum GHOST
		 */
		if (MAP[heroi_y][heroi_x].iFlags & BLOCK_GHOST) {
			TorturaHeroi();
		}

		/**
		 * Verifica se existe um LUNCH para comer
		 */
		if (MAP[heroi_y][heroi_x].iFlags & BLOCK_RIDER_LUNCH) {
			MAP[heroi_y][heroi_x].iFlags &= ~BLOCK_RIDER_LUNCH;
			num_lunch--;
			
			if (!num_lunch) {
				MessageBoxA(0, "VIVA!!!!", ":D", 0);
				exit(0);
			}
		}

    }
}

void TrataTeclas(GLubyte tecla, GLint x_mouse, GLint y_mouse) {
    //- 27 = ESC
    if (tecla == 27 ) exit(0); 
}


//- Programa Principal 
int main(void)
{
	Inicializa();

	glutDisplayFunc(Desenha);
	glutSpecialFunc(TeclasEspeciais);
	glutKeyboardFunc(TrataTeclas);
	continue_timer = 1;
	glutTimerFunc(500, TimerAnimaGhost, 0);
	glutMainLoop();
}
