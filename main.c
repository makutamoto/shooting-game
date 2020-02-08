#include<stdio.h>
#include<math.h>
#include<time.h>
#include<Windows.h>

#include "./cnsglib/include/cnsg.h"

#define WIDTH 512
#define HEIGHT 256
#define FRAME_PER_SECOND 60

#define HERO_BULLET_COLLISIONMASK 0x01
#define ENEMY_BULLET_COLLISIONMASK 0x02
#define OBSTACLE_COLLISIONMASK 0x04

static struct {
	float move[2];
	float direction[2];
	float action;
	float retry;
	float quit;
} controller;

static FontSJIS shnm12;

static float autoMove[2];
static BOOL autoAction;

static Image lifeBarBunch;
static Image lifebarImage;
static Image hero;
static Image heroBullet;
static Image enemy1;
static Image stoneImage;
static Image explosionImage;

static Shape enemy1Shape;
static Shape enemy1CollisionShape;
static Shape enemyLifeShape;
static Shape heroBulletShape;
static Shape enemyBulletShape;
static Shape stoneShape;
static Shape explosionShape;

static Node lifeBarNode;
static Node scoreNode;
static Scene gameScene;
static Node heroNode;
static Node rayNode;
static Node gameoverNode;
static Node startNode;
static Node skyboxNode;

BOOL start = TRUE;
static unsigned int heroHP;
static unsigned int score;
static IntervalEventScene *gameSceneIntervalEvent;

typedef struct {
	int i;
	float dx;
} Explosion;

typedef struct {
	unsigned int hp;
	Node *bar;
} Enemy1;

static int explosionInterval(Node *node, void *data) {
	Explosion *explosion = node->data;
	if(explosion->i < 5) {
		node->scale[0] += explosion->dx;
		node->scale[1] += explosion->dx;
		node->scale[2] += explosion->dx;
		explosion->i += 1;
	} else {
		removeByData(&gameScene.nodes, node);
		free(node);
	}
	return TRUE;
}

static void causeExplosion(float position[3], float radius) {
	Node *explosionNode = malloc(sizeof(Node));
	Explosion *explosion = calloc(sizeof(Explosion), 1);
	*explosionNode = initNode("explosion", explosionImage);
	explosionNode->shape = explosionShape;
	explosionNode->collisionShape = explosionShape;
	explosionNode->velocity[2] = -100.0F;
	explosionNode->position[0] = position[0];
	explosionNode->position[1] = position[1];
	explosionNode->position[2] = position[2];
	explosionNode->scale[0] = 0.0F;
	explosionNode->scale[1] = 0.0F;
	explosionNode->scale[2] = 0.0F;
	explosionNode->data = explosion;
	explosion->dx = radius / 5.0F;
	addIntervalEventNode(explosionNode, 0.1F, explosionInterval, NULL);
	push(&gameScene.nodes, explosionNode);
	PlaySoundNeo("./assets/se_maoudamashii_retro12.wav", FALSE);
}

static int bulletBehaviour(Node *node, float elapsed) {
	if(node->collisionFlags || distance2(heroNode.position, node->position) > 500 || node->position[2] < -100.0F) {
		removeByData(&gameScene.nodes, node);
		free(node);
		return FALSE;
	}
	return TRUE;
}

static void shootBullet(const char *name, Shape shape, const float position[3], float angle, unsigned int collisionMask) {
	Node *bullet = malloc(sizeof(Node));
	*bullet = initNode(name, NO_IMAGE);
	bullet->shape = shape;
	bullet->collisionShape = shape;
	bullet->velocity[0] = 400.0F * cosf(angle - PI / 2.0F);
	bullet->velocity[2] = -400.0F * sinf(angle - PI / 2.0F);
	bullet->angle[1] = angle;
	bullet->position[0] = position[0];
	bullet->position[1] = position[1];
	bullet->position[2] = position[2];
	bullet->collisionMaskActive = collisionMask;
	bullet->behaviour = bulletBehaviour;
	push(&gameScene.nodes, bullet);
}

static int enemy1Behaviour(Node *node, float elapsed) {
	if(node->position[2] < -100.0F) {
		removeByData(&gameScene.nodes, node);
		free(node);
		return FALSE;
	}
	if(node->collisionFlags & HERO_BULLET_COLLISIONMASK) {
		Enemy1 *enemy = node->data;
		enemy->hp -= 1;
		cropImage(&enemy->bar->texture, &lifeBarBunch, 0, 10 * enemy->hp / 1);
		if(enemy->hp <= 0) {
			causeExplosion(node->position, 50.0F);
			removeByData(&gameScene.nodes, node);
			free(node);
			score += 10;
			return FALSE;
		}
	}
	return TRUE;
}

static int enemy1BehaviourInterval(Node *node, void *data) {
	shootBullet("enemyBullet", enemyBulletShape, node->position, PI, ENEMY_BULLET_COLLISIONMASK);
	return TRUE;
}

static void spawnEnemy1(float x, float y, float z) {
	Node *enemy = malloc(sizeof(Node));
	Node *bar = malloc(sizeof(Node));
	Enemy1 *data = malloc(sizeof(Enemy1));
	Image image = initImage(192, 32, BLACK, NULL_COLOR);
	*enemy = initNode("enemy1", enemy1);
	cropImage(&image, &lifeBarBunch, 0, 10);
	*bar = initNode("EnemyLifeBar", image);
	data->hp = 1;
	data->bar = bar;
	enemy->shape = enemy1Shape;
	enemy->collisionShape = enemy1CollisionShape;
	enemy->collisionShape = enemy1Shape;
	enemy->position[0] = x;
	enemy->position[1] = y;
	enemy->position[2] = z;
	enemy->velocity[2] = -100.0F;
	enemy->scale[0] = 50.0F;
	enemy->scale[1] = 50.0F;
	enemy->scale[2] = 50.0F;
	enemy->collisionMaskActive = OBSTACLE_COLLISIONMASK;
	enemy->collisionMaskPassive = HERO_BULLET_COLLISIONMASK;
	enemy->behaviour = enemy1Behaviour;
	enemy->data = data;
	addIntervalEventNode(enemy, 3.0F, enemy1BehaviourInterval, NULL);
	bar->shape = enemyLifeShape;
	bar->position[1] = -16.0F / 50.0F;
	push(&enemy->children, bar);
	push(&gameScene.nodes, enemy);
}

static int stoneBehaviour(Node *node, float elapsed) {
	if(node->position[2] < -100.0F) {
		removeByData(&gameScene.nodes, node);
		free(node);
		return FALSE;
	}
	return TRUE;
}

static void spawnStone(float x, float y, float z) {
	Node *stone = malloc(sizeof(Node));
	*stone = initNode("stone", stoneImage);
	stone->shape = stoneShape;
	stone->collisionShape = stoneShape;
	stone->position[0] = x;
	stone->position[1] = y;
	stone->position[2] = z;
	stone->velocity[2] = -100.0F;
	stone->scale[0] = 32.0F;
	stone->scale[1] = 32.0F;
	stone->scale[2] = 32.0F;
	stone->behaviour = stoneBehaviour;
	stone->collisionMaskActive = OBSTACLE_COLLISIONMASK;
	stone->collisionMaskPassive = HERO_BULLET_COLLISIONMASK | ENEMY_BULLET_COLLISIONMASK;
	push(&gameScene.nodes, stone);
}

static void gameover(void) {
	if(!start) {
		push(&gameScene.nodes, &gameoverNode);
	}
}

static void autoControl(void) {
	Node *node;
	autoMove[0] = 0.0F;
	autoMove[1] = 0.0F;
	resetIteration(&gameScene.nodes);
	node = nextData(&gameScene.nodes);
	while(node) {
		float temp[2];
		if(strcmp("enemy1", node->id) == 0) {
			subVec2(node->position, heroNode.position, temp);
			if(node->position[2] > 200.0F) {
				addVec2(autoMove, temp, autoMove);
				autoAction = TRUE;
				break;
			} else {
				subVec2(autoMove, node->position, autoMove);
			}
		} else if(strcmp("stone", node->id) == 0 || strcmp("enemyBullet", node->id) == 0) {
			if(node->position[2] < 200.0F) {
				subVec2(autoMove, node->position, autoMove);
			}
		}
		node = nextData(&gameScene.nodes);
	}
	normalize2(autoMove, autoMove);
}

static int heroBehaviour(Node *node, float elapsed) {
	float move[2];
	if(node->collisionFlags) {
		if(node->collisionFlags & ENEMY_BULLET_COLLISIONMASK) heroHP -= 1;
		if(node->collisionFlags & OBSTACLE_COLLISIONMASK) heroHP = 0;
		cropImage(&lifebarImage, &lifeBarBunch, 0, heroHP);
		if(heroHP <= 0) {
			causeExplosion(node->position, 50.0F);
			removeByData(&gameScene.nodes, node);
			gameover();
		}
	}
	if(start) {
		mulVec2ByScalar(autoMove, 200.0F, move);
	} else {
		mulVec2ByScalar(controller.move, 200.0F, move);
	}
	node->velocity[0] = move[0];
	node->velocity[1] = move[1];
	node->position[0] = max(min(node->position[0], 100.0F), -100.0F);
	node->position[1] = max(min(node->position[1], 100.0F), -100.0F);
	if(controller.action || (start && autoAction)) {
		static clock_t previousClock;
		if((clock() - previousClock) * 1000 / CLOCKS_PER_SEC > 500) {
			shootBullet("heroBullet", heroBulletShape, node->position, node->angle[1], HERO_BULLET_COLLISIONMASK);
			controller.action = FALSE;
			PlaySoundNeo("assets/laser.wav", FALSE);
			previousClock = clock();
		}
	}
	return TRUE;
}

static int scoreBehaviour(Node *node, float elapsed) {
	char buffer[11];
	sprintf(buffer, "SCORE %04u", score);
	drawTextSJIS(&node->texture, &shnm12, 0, 0, buffer);
	return TRUE;
}

static int gameSceneInterval() {
	if(heroHP > 0) {
		int x, y;
		for(x = 0;x < 3;x++) {
			for(y = 0;y < 3;y++) {
				float cx = 200.0F / 3.0F * x - 100.0F;
				float cy = 200.0F / 3.0F * y - 60.0F;
				switch(rand() % 4) {
					case 0:
						spawnStone(cx, cy, 500.0F);
						break;
					case 1:
						spawnEnemy1(cx, cy, 500.0F);
						break;
					case 2:
					case 3:
						break;
				}
			}
		}
		score += 5;
	}
	return TRUE;
}

static void startGame(void) {
	heroHP = 10;
	cropImage(&lifebarImage, &lifeBarBunch, 0, 10);
	clearVec3(heroNode.position);

	srand(0);
	gameSceneIntervalEvent->counter = 0.0F;

	clearVector(&gameScene.nodes);
	if(start) push(&gameScene.nodes, &startNode);
	push(&gameScene.nodes, &lifeBarNode);
	push(&gameScene.nodes, &scoreNode);
	push(&gameScene.nodes, &heroNode);
	push(&gameScene.nodes, &skyboxNode);
	gameSceneInterval();
	score = 0;
}

static void gameSceneBehaviour(Scene *scene, float elapsed) {
	static float cameraBase[3] = { 0.0F, 25.0F, -100.0F };
	float tempVec3[1][3];
	mulVec3ByScalar(subVec3(heroNode.position, subVec3(scene->camera.position, cameraBase, tempVec3[0]), tempVec3[0]), 5.0F * elapsed, tempVec3[0]);
	addVec3(scene->camera.position, tempVec3[0], scene->camera.position);
	copyVec3(scene->camera.target, scene->camera.position);
	scene->camera.target[2] = 0.0F;
}

static void initGame(void) {
	shnm12 = initFontSJIS(loadBitmap("assets/shnm6x12r.bmp", NULL_COLOR), loadBitmap("assets/shnmk12.bmp", NULL_COLOR), 6, 12, 12);

	initControllerDataCross(NULL, 'W', 'A', 'S', 'D', controller.move);
	initControllerData(VK_SPACE, 1.0F, 0.0F, &controller.action);
	initControllerEvent('R', NULL, startGame);
	initControllerEvent(VK_ESCAPE, NULL, deinitCNSG);

	lifeBarBunch = loadBitmap("assets/lifebar.bmp", NULL_COLOR);
	lifebarImage = initImage(192, 32, BLACK, NULL_COLOR);
	cropImage(&lifebarImage, &lifeBarBunch, 0, 10);
	hero = loadBitmap("assets/hero3d.bmp", NULL_COLOR);
	heroBullet = loadBitmap("assets/heroBullet.bmp", WHITE);
	enemy1 = loadBitmap("assets/enemy1.bmp", NULL_COLOR);
	stoneImage = loadBitmap("assets/stone.bmp", NULL_COLOR);
	explosionImage = loadBitmap("./assets/explosion.bmp", NULL_COLOR);
	gameScene = initScene();
	setCurrentScene(&gameScene, NULL, 0.0F);
	gameScene.camera = initCamera(0.0F, 0.0F, -100.0F / 32.0F);
	gameScene.camera.farLimit = 2000.0F;
	gameScene.background = BLACK;
	gameScene.behaviour = gameSceneBehaviour;
	gameSceneIntervalEvent = addIntervalEventScene(&gameScene, 5.0F, gameSceneInterval, NULL);
	enemy1CollisionShape = initShapeBox(1.0F, 2.0F, 1.0F, MAGENTA);
	enemy1Shape = loadObj("./assets/enemy1.obj");
	stoneShape = loadObj("./assets/stone.obj");
	explosionShape = loadObj("./assets/explosion.obj");
	enemyLifeShape = initShapePlane(20.0F / 50.0F, 5.0F / 50.0F, RED);
	heroBulletShape = initShapeBox(5, 5, 30, YELLOW);
	enemyBulletShape = initShapeBox(5, 5, 30, MAGENTA);
	lifeBarNode = initNodeUI("lifeBarNode", lifebarImage, BLACK);
	lifeBarNode.position[0] = 2.5F;
	lifeBarNode.position[1] = 2.5F;
	lifeBarNode.scale[0] = 30.0F;
	lifeBarNode.scale[1] = 5.0F;
	heroNode = initNode("Hero", hero);
	heroNode.shape = loadObj("./assets/hero.obj");
	heroNode.collisionShape = heroNode.shape;
	heroNode.scale[0] = 32.0F;
	heroNode.scale[1] = 32.0F;
	heroNode.scale[2] = 32.0F;
	heroNode.collisionMaskPassive = ENEMY_BULLET_COLLISIONMASK | OBSTACLE_COLLISIONMASK;
	heroNode.behaviour = heroBehaviour;
	rayNode = initNode("ray", NO_IMAGE);
	rayNode.shape = initShapeBox(3.0F / 32.0F, 3 / 32.0F, 512 / 32.0F, RED);
	rayNode.position[2] = 256.0F / 32.0F;
	push(&heroNode.children, &rayNode);

	skyboxNode = initNode("skybox", loadBitmap("./assets/skybox.bmp", MAGENTA));
	skyboxNode.shape = loadObj("./assets/skybox.obj");
	setVec3(skyboxNode.scale, 1000.0F, XYZ_MASK);

	startNode = initNodeText("gameover", 0, 0, CENTER, CENTER, 96, 48, NULL);
	drawTextSJIS(&startNode.texture, &shnm12, 0, 0, "SPACE SHOOTER\n\n\"SPACE\"でプレイ\n\"ESC\"で終了");

	scoreNode = initNodeText("score", 0, 0, RIGHT, TOP, 60, 12, scoreBehaviour);

	gameoverNode = initNodeText("gameover", 0, 0, CENTER, CENTER, 84, 48, NULL);
	drawTextSJIS(&gameoverNode.texture, &shnm12, 0, 0, "ゲームオーバー\n\n\"R\"でリプレイ\n\"ESC\"で終了");
}

int main(int argc, char *argv[]) {
	initCNSG(argc, argv, WIDTH, HEIGHT);
	initGame();
	start = FALSE;
	startGame();
	gameLoop(FRAME_PER_SECOND);
	// 		if(controller.action) {
	// 			start = FALSE;
	// 			startGame();
	// 		} else {
	// 			autoControl();
	// 		}
	return 0;
}
