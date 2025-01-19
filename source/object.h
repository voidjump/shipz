#ifndef SHIPZ_OBJECTS_H
#define SHIPZ_OBJECTS_H
#include <SDL3/SDL.h>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include "sync.h"

enum OBJECT_TYPE {
    BULLET,
    ROCKET,
    MINE,
    PLAYER,
    BASE,
};

class Object {
    protected:
        // All objects
        static std::unordered_map<Uint16, std::shared_ptr<Object>> instances;

        // Function called when destroyed
        std::function<void()> destroy_callback;

        // Function called when updated
        std::function<void(float)> update_callback;

        // Function called when remotely synced
        std::function<void(SyncObjectUpdate*)> sync_callback;

    public:
        Uint16 id;
        Uint8 type;

        Object(Uint16 id, Uint8 type);

        // Spawn an object based on remote description
        void HandleSpawn(SyncObjectSpawn *sync);

        // Destroy this object, calling Destroy callback
        void HandleDestroy(SyncObjectDestroy *sync);

        // Handle update packet, calling Sync callback
        void HandleSync(SyncObjectUpdate *sync);

        // Destroy local instance, calling Destroy callback
        void Destroy();

        // Retrieve object instance by ID
        static std::shared_ptr<Object> Object::GetByID(Uint16 search_id);

        // Local update, calling Update callback
        void Update(float delta);
};

#endif