#include "object.h"
#include "log.h"
#include "bullet.h"

Object::Object(Uint16 id, Uint8 type) {
    this->id = id;
    this->type = type;
}
 
 // Handle spawn message
void Object::HandleSpawn(SyncObjectSpawn *sync) {
    auto it = instances.find(this->id);
    if (it != instances.end()) {
        log::error("Refusing to spawn object with id ", sync->id, " as it alreayd exists;");
        return;
    }
    // Call appropriate constructor based on object type
    switch(sync->type) {
        case OBJECT_TYPE::BULLET:
            instances[sync->id] = std::make_shared<Bullet>(sync);
            break;
        case OBJECT_TYPE::ROCKET:
            instances[sync->id] = std::make_shared<Rocket>(sync);
            break;
        case OBJECT_TYPE::MINE:
            instances[sync->id] = std::make_shared<Mine>(sync);
            break;
        default:
            log::debug("Cannot spawn unknown object type ", sync->type);
    }
}

// Handle destroy message
void Object::HandleDestroy(SyncObjectDestroy *sync) {
    auto instance = Object::GetByID(sync->id) ;
    if(!instance) {
        return;
    }
    instance->Destroy();
}

// Handle sync message
void Object::HandleSync(SyncObjectUpdate *sync) {
    auto instance = Object::GetByID(sync->id) ;
    if(!instance) {
        return;
    }
    if( !instance->sync_callback ) {
        log::debug( "Received sync instruction for object without a sync callback");
        return;
    }
    instance->sync_callback(sync);
}

// Remove object, this calls its destroy_callback, if any is registered 
void Object::Destroy() {
    if( this->destroy_callback) {
        destroy_callback();
    }
    // Remove this object from instances
    auto it = instances.find(this->id);
    if (it != instances.end()) {
        instances.erase(it);
    }
}

// Retrieve an object instance by ID
std::shared_ptr<Object> Object::GetByID(Uint16 search_id) {
    auto it = instances.find(search_id);
    if (it != instances.end()) {
        return it->second;
    }
    return nullptr;
}

// Update object, this calls update_callback, if any is registered
void Object::Update(float delta) {
    if( this->update_callback) {
        update_callback(delta);
    }
}
