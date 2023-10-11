#include "Game.hpp"

#include "Connection.hpp"

#include <stdexcept>
#include <iostream>
#include <cstring>

#include <glm/gtx/norm.hpp>

Ball * Game::spawn_ball(){
	balls.emplace_back();
	Ball &ball = balls.back();

	ball.position.x = glm::mix(ArenaMin.x + 2.0f * BallRadius, ArenaMax.x - 2.0f * BallRadius, 0.4f + 0.2f * mt() / float(mt.max()));
	ball.position.y = glm::mix(ArenaMin.y + 2.0f * BallRadius, ArenaMax.y - 2.0f * BallRadius, 0.4f + 0.2f * mt() / float(mt.max()));

	ball.velocity = glm::vec2{0.0f};

	do {
		ball.color.r = mt() / float(mt.max());
		ball.color.g = mt() / float(mt.max());
		ball.color.b = mt() / float(mt.max());
	} while (ball.color == glm::vec3(0.0f));
	ball.color = glm::normalize(ball.color);

	return &ball;
}

void Player::Controls::send_controls_message(Connection *connection_) const {
	assert(connection_);
	auto &connection = *connection_;

	uint32_t size = 5;
	connection.send(Message::C2S_Controls);
	connection.send(uint8_t(size));
	connection.send(uint8_t(size >> 8));
	connection.send(uint8_t(size >> 16));

	auto send_button = [&](Button const &b) {
		if (b.downs & 0x80) {
			std::cerr << "Wow, you are really good at pressing buttons!" << std::endl;
		}
		connection.send(uint8_t( (b.pressed ? 0x80 : 0x00) | (b.downs & 0x7f) ) );
	};

	send_button(left);
	send_button(right);
	send_button(up);
	send_button(down);
	send_button(jump);
}

bool Player::Controls::recv_controls_message(Connection *connection_) {
	assert(connection_);
	auto &connection = *connection_;

	auto &recv_buffer = connection.recv_buffer;

	//expecting [type, size_low0, size_mid8, size_high8]:
	if (recv_buffer.size() < 4) return false;
	if (recv_buffer[0] != uint8_t(Message::C2S_Controls)) return false;
	uint32_t size = (uint32_t(recv_buffer[3]) << 16)
	              | (uint32_t(recv_buffer[2]) << 8)
	              |  uint32_t(recv_buffer[1]);
	if (size != 5) throw std::runtime_error("Controls message with size " + std::to_string(size) + " != 5!");
	
	//expecting complete message:
	if (recv_buffer.size() < 4 + size) return false;

	auto recv_button = [](uint8_t byte, Button *button) {
		button->pressed = (byte & 0x80);
		uint32_t d = uint32_t(button->downs) + uint32_t(byte & 0x7f);
		if (d > 255) {
			std::cerr << "got a whole lot of downs" << std::endl;
			d = 255;
		}
		button->downs = uint8_t(d);
	};

	recv_button(recv_buffer[4+0], &left);
	recv_button(recv_buffer[4+1], &right);
	recv_button(recv_buffer[4+2], &up);
	recv_button(recv_buffer[4+3], &down);
	recv_button(recv_buffer[4+4], &jump);

	//delete message from buffer:
	recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + 4 + size);

	return true;
}


//-----------------------------------------

Game::Game() : mt(0x15466666) {
	connected = std::make_pair(false,false);
}

Player *Game::spawn_player() {
	players.emplace_back();
	Player &player = players.back();

	// Someone get disconnected
	if(next_player_number > 2){
		connected = std::make_pair(false,false);
		for (auto &p : players){
			if(p.idx == 1){
				connected.first	= true;
			}
			else if(p.idx == 2){
				connected.second = true;
			}
		}

		if(connected.first == false){
			player.idx = 1;
			connected.first	= true;
		}else if(connected.second == false){
			player.idx = 2;
			connected.second = true;
		}

	}else{
		// Normal cases
		player.idx = next_player_number;
		if(next_player_number == 1){
			connected.first	= true;
		}else if(next_player_number == 2){
			connected.second = true;
		}
	}

	

	if(player.idx == 1){
		printf("Spawn player %d\n",player.idx);
		// player.selfArenaMin.x = ArenaMin.x;
		// player.selfArenaMin.y = ArenaMin.y;
		// player.selfArenaMax.x = ArenaMax.x;
		// player.selfArenaMax.y = (ArenaMax.y + ArenaMin.y) / 2;
		player.position.x = glm::mix(ArenaMin1.x + 2.0f * PlayerRadius, ArenaMax1.x - 2.0f * PlayerRadius, 0.4f + 0.2f * mt() / float(mt.max()));
		player.position.y = glm::mix(ArenaMin1.y + 2.0f * PlayerRadius, ArenaMax1.y - 2.0f * PlayerRadius, 0.4f + 0.2f * mt() / float(mt.max()));
	}else{
		// player.selfArenaMin.x = ArenaMin.x;
		// player.selfArenaMin.y = (ArenaMax.y + ArenaMin.y) / 2;
		// player.selfArenaMax.x = ArenaMax.x;
		// player.selfArenaMax.y = ArenaMax.y;
		printf("Spawn player %d\n",player.idx);
		player.position.x = glm::mix(ArenaMin2.x + 2.0f * PlayerRadius, ArenaMax2.x - 2.0f * PlayerRadius, 0.4f + 0.2f * mt() / float(mt.max()));
		player.position.y = glm::mix(ArenaMin2.y + 2.0f * PlayerRadius, ArenaMax2.y - 2.0f * PlayerRadius, 0.4f + 0.2f * mt() / float(mt.max()));
	}

	//random point in the middle area of the arena:


	do {
		player.color.r = mt() / float(mt.max());
		player.color.g = mt() / float(mt.max());
		player.color.b = mt() / float(mt.max());
	} while (player.color == glm::vec3(0.0f));
	player.color = glm::normalize(player.color);


	player.name = "P" + std::to_string(player.idx);

	next_player_number ++;

	return &player;
}

void Game::remove_player(Player *player) {
	bool found = false;
	for (auto pi = players.begin(); pi != players.end(); ++pi) {
		if (&*pi == player) {
			players.erase(pi);
			found = true;
			break;
		}
	}
	assert(found);
}

void Game::update(float elapsed) {
	//position/velocity update:
	for (auto &p : players) {
		glm::vec2 dir = glm::vec2(0.0f, 0.0f);
		if (p.controls.left.pressed) dir.x -= 1.0f;
		if (p.controls.right.pressed) dir.x += 1.0f;
		if (p.controls.down.pressed) dir.y -= 1.0f;
		if (p.controls.up.pressed) dir.y += 1.0f;

		if (dir == glm::vec2(0.0f)) {
			//no inputs: just drift to a stop
			float amt = 1.0f - std::pow(0.5f, elapsed / (PlayerAccelHalflife * 2.0f));
			p.velocity = glm::mix(p.velocity, glm::vec2(0.0f,0.0f), amt);
		} else {
			//inputs: tween velocity to target direction
			dir = glm::normalize(dir);

			float amt = 1.0f - std::pow(0.5f, elapsed / PlayerAccelHalflife);

			//accelerate along velocity (if not fast enough):
			float along = glm::dot(p.velocity, dir);
			if (along < PlayerSpeed) {
				along = glm::mix(along, PlayerSpeed, amt);
			}

			//damp perpendicular velocity:
			float perp = glm::dot(p.velocity, glm::vec2(-dir.y, dir.x));
			perp = glm::mix(perp, 0.0f, amt);

			p.velocity = dir * along + glm::vec2(-dir.y, dir.x) * perp;
		}
		p.position += p.velocity * elapsed;

		//reset 'downs' since controls have been handled:
		p.controls.left.downs = 0;
		p.controls.right.downs = 0;
		p.controls.up.downs = 0;
		p.controls.down.downs = 0;
		p.controls.jump.downs = 0;
	}

	static float friction_factor = 0.5;

	for (auto &b: balls){
		//glm::vec2 dir = glm::normalize(b.velocity);
		glm::vec2 friction = -b.velocity * friction_factor;
		b.position += b.velocity * elapsed + friction * elapsed * elapsed * 0.5f;
		b.position += b.velocity * elapsed;
		b.velocity += friction * elapsed;
	}

	//collision resolution:
	for (auto &p1 : players) {
		//player/player collisions:
		// No need to worry about this
		for (auto &p2 : players) {
			if (&p1 == &p2) break;
			glm::vec2 p12 = p2.position - p1.position;
			float len2 = glm::length2(p12);
			if (len2 > (2.0f * PlayerRadius) * (2.0f * PlayerRadius)) continue;
			if (len2 == 0.0f) continue;
			glm::vec2 dir = p12 / std::sqrt(len2);
			//mirror velocity to be in separating direction:
			glm::vec2 v12 = p2.velocity - p1.velocity;
			glm::vec2 delta_v12 = dir * glm::max(0.0f, -1.75f * glm::dot(dir, v12));
			p2.velocity += 0.5f * delta_v12;
			p1.velocity -= 0.5f * delta_v12;
		}

		glm::vec2 curArenaMin{0.0f};
		glm::vec2 curArenaMax{0.0f};

		if(p1.idx == 1){
			curArenaMin = ArenaMin1;
			curArenaMax = ArenaMax1;
		}else if(p1.idx == 2){
			curArenaMin = ArenaMin2;
			curArenaMax = ArenaMax2;
		}else{
			std::runtime_error("What index??");
		}

		//player/arena collisions:
		if (p1.position.x < curArenaMin.x + PlayerRadius) {
			p1.position.x = curArenaMin.x + PlayerRadius;
			p1.velocity.x = std::abs(p1.velocity.x);
		}
		if (p1.position.x > curArenaMax.x - PlayerRadius) {
			p1.position.x = curArenaMax.x - PlayerRadius;
			p1.velocity.x =-std::abs(p1.velocity.x);
		}
		if (p1.position.y < curArenaMin.y + PlayerRadius) {
			p1.position.y = curArenaMin.y + PlayerRadius;
			p1.velocity.y = std::abs(p1.velocity.y);
		}
		if (p1.position.y > curArenaMax.y - PlayerRadius) {
			p1.position.y = curArenaMax.y - PlayerRadius;
			p1.velocity.y =-std::abs(p1.velocity.y);
		}
	}

	for (auto &b : balls){
		// Collision with one of the player
		for(auto &p : players){
			glm::vec2 l = p.position - b.position;
			float len2 = glm::length2(l);
			if(len2 > (PlayerRadius + BallRadius) * (PlayerRadius + BallRadius)) continue;
			if(len2 == 0.0f) continue;
			glm::vec2 dir = l / std::sqrt(len2);
			glm::vec2 v = p.velocity - b.velocity;
			glm::vec2 delta_v = dir * glm::max(0.0f, -1.75f * glm::dot(dir,v));
			p.velocity += 0.5f * delta_v;
			b.velocity -= 0.5f * delta_v;
		}

		if (b.position.x < ArenaMin.x + PlayerRadius) {
			b.position.x = ArenaMin.x + PlayerRadius;
			b.velocity.x = std::abs(b.velocity.x);
		}
		if (b.position.x > ArenaMax.x - PlayerRadius) {
			b.position.x = ArenaMax.x - PlayerRadius;
			b.velocity.x =-std::abs(b.velocity.x);
		}


		auto add_score = [&](uint8_t idx){
			for(auto &p : players){
				if(p.idx == idx){
					p.score += 1;
				}
			}
		};

		// idx means who goals
		auto reset_ball = [&](uint8_t idx){
			// let the ball respawn to opposite player side
			glm::vec2 curArenaMin{0.0f};
			glm::vec2 curArenaMax{0.0f};

			if(idx==1){
				curArenaMin = ArenaMin2;
				curArenaMax = ArenaMax2;
			}else if(idx == 2){
				curArenaMin = ArenaMin1;
				curArenaMax = ArenaMax1;
			}else{
				std::runtime_error("!");
			}

			auto touch_player = [&](Ball b, uint8_t idx)->bool{
				Player player = players.front();
				for(auto &p : players){
					if(p.idx != idx){
						player = p;
						break;
					}
				}

				glm::vec2 l = player.position - b.position;
				float len2 = glm::length2(l);
				if(len2 > ((PlayerRadius + BallRadius) * (PlayerRadius + BallRadius) + 0.05)) return false;
				else return true;

			};

			auto &b = balls.front();
			b.velocity = glm::vec2{0.0f};
			do {
				b.position.x = glm::mix(curArenaMin.x + 2.0f * BallRadius, curArenaMax.x - 2.0f * BallRadius, 0.4f + 0.2f * mt() / float(mt.max()));
				b.position.y = glm::mix(curArenaMin.y + 2.0f * BallRadius, curArenaMax.y - 2.0f * BallRadius, 0.4f + 0.2f * mt() / float(mt.max()));
			} while(touch_player(b,idx));
			
			do {
				b.color.r = mt() / float(mt.max());
				b.color.g = mt() / float(mt.max());
				b.color.b = mt() / float(mt.max());
			} while (b.color == glm::vec3(0.0f));
			b.color = glm::normalize(b.color);
		};

		// Goal Situation
		// player2 scores
		if (b.position.y < ArenaMin.y + PlayerRadius) {
			b.position.y = ArenaMin.y + PlayerRadius;
			b.velocity.y = std::abs(b.velocity.y);
			add_score(2);
			reset_ball(2);
		}
		// player1 scores
		if (b.position.y > ArenaMax.y - PlayerRadius) {
			b.position.y = ArenaMax.y - PlayerRadius;
			b.velocity.y =-std::abs(b.velocity.y);
			add_score(1);
			reset_ball(1);
		}
	}

	// Check if any player wins
	for(auto &p: players){
		if(p.score == 5){
			winner_idx = p.idx;
			break;
		}
	}

}


void Game::send_state_message(Connection *connection_, Player *connection_player) const {
	assert(connection_);
	auto &connection = *connection_;

	connection.send(Message::S2C_State);
	//will patch message size in later, for now placeholder bytes:
	connection.send(uint8_t(0));
	connection.send(uint8_t(0));
	connection.send(uint8_t(0));
	size_t mark = connection.send_buffer.size(); //keep track of this position in the buffer


	//send player info helper:
	auto send_player = [&](Player const &player) {
		connection.send(player.position);
		connection.send(player.velocity);
		connection.send(player.color);
		connection.send(player.score);
		connection.send(player.idx);
	
		//NOTE: can't just 'send(name)' because player.name is not plain-old-data type.
		//effectively: truncates player name to 255 chars
		uint8_t len = uint8_t(std::min< size_t >(255, player.name.size()));
		connection.send(len);
		connection.send_buffer.insert(connection.send_buffer.end(), player.name.begin(), player.name.begin() + len);
	};

	auto send_ball = [&](Ball const &ball){
		connection.send(ball.position);
		connection.send(ball.velocity);
		connection.send(ball.color);
	};

	//player count:
	connection.send(uint8_t(players.size()));
	if (connection_player) send_player(*connection_player);
	for (auto const &player : players) {
		if (&player == connection_player) continue;
		send_player(player);
	}

	connection.send(uint8_t(balls.size()));
	for (auto const &ball : balls){
		send_ball(ball);
	}

	connection.send(winner_idx);

	//compute the message size and patch into the message header:
	uint32_t size = uint32_t(connection.send_buffer.size() - mark);
	connection.send_buffer[mark-3] = uint8_t(size);
	connection.send_buffer[mark-2] = uint8_t(size >> 8);
	connection.send_buffer[mark-1] = uint8_t(size >> 16);
}

bool Game::recv_state_message(Connection *connection_) {
	assert(connection_);
	auto &connection = *connection_;
	auto &recv_buffer = connection.recv_buffer;

	if (recv_buffer.size() < 4) return false;
	if (recv_buffer[0] != uint8_t(Message::S2C_State)) return false;
	uint32_t size = (uint32_t(recv_buffer[3]) << 16)
	              | (uint32_t(recv_buffer[2]) << 8)
	              |  uint32_t(recv_buffer[1]);
	uint32_t at = 0;
	//expecting complete message:
	if (recv_buffer.size() < 4 + size) return false;

	//copy bytes from buffer and advance position:
	auto read = [&](auto *val) {
		if (at + sizeof(*val) > size) {
			throw std::runtime_error("Ran out of bytes reading state message.");
		}
		std::memcpy(val, &recv_buffer[4 + at], sizeof(*val));
		at += sizeof(*val);
	};

	players.clear();
	uint8_t player_count;
	read(&player_count);
	for (uint8_t i = 0; i < player_count; ++i) {
		players.emplace_back();
		Player &player = players.back();
		read(&player.position);
		read(&player.velocity);
		read(&player.color);
		read(&player.score);
		read(&player.idx);
		uint8_t name_len;
		read(&name_len);
		//n.b. would probably be more efficient to directly copy from recv_buffer, but I think this is clearer:
		player.name = "";
		for (uint8_t n = 0; n < name_len; ++n) {
			char c;
			read(&c);
			player.name += c;
		}
	}
	balls.clear();
	uint8_t ball_count;
	read(&ball_count);
	for (uint8_t i = 0; i < ball_count; ++i) {
		balls.emplace_back();
		Ball &ball = balls.back();
		read(&ball.position);
		read(&ball.velocity);
		read(&ball.color);
	}

	read(&winner_idx);

	if (at != size) throw std::runtime_error("Trailing data in state message.");

	//delete message from buffer:
	recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + 4 + size);

	return true;
}
