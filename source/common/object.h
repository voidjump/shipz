#ifndef SHIPZ_OBJECTS_H
#define SHIPZ_OBJECTS_H
#include <SDL3/SDL.h>
#include <vector>
#include <cstdint>
#include <type_traits>
#include <map>
#include <memory>
#include <functional>
#include "messages/event.h"
#include "messages/sync.h"

constexpr uint16_t SERVER_SHOULD_DEFINE_ID = 0;
constexpr uint16_t NO_OBJECT_ID_AVAILABLE = UINT16_MAX;

enum OBJECT_TYPE {
    BULLET,
    ROCKET,
    MINE,
    PLAYER,
    BASE,
};

template <typename T>
void append_to_object(std::vector<uint8_t>& buffer, T value) {
    static_assert(std::is_arithmetic<T>::value, "T must be an arithmetic type");
    size_t size = sizeof(T);
    uint8_t* data = reinterpret_cast<uint8_t*>(&value);
    for (size_t i = 0; i < size; ++i) {
        buffer.push_back(data[i]);
    }
}

using ObjectID = uint16_t;
using ObjectType = uint8_t;

class Object {
    protected:
        // All objects
        static std::unordered_map<ObjectID, std::shared_ptr<Object>> instances;

        // Function called when destroyed
        std::function<void()> destroy_callback;

        // Function called when updated
        std::function<void(float)> update_callback;

        // Function called when remotely synced
        std::function<void(SyncObjectUpdate*)> sync_callback;

    public:
        ObjectID id;
        ObjectType type;

        // Constructor
        Object(ObjectID id, ObjectType type);

        // Constructor that auto assigns ID (use server side only)
        Object(ObjectType type);

        // Ensure the base class is polymorphic
        virtual ~Object() {} 

        // Spawn an object based on remote description
        static void HandleSpawn(EventObjectSpawn *sync);

        // Destroy this object, calling Destroy callback
        void HandleDestroy(EventObjectDestroy *sync);

        // Handle update packet, calling Sync callback
        void HandleSync(SyncObjectUpdate *sync);

        // Destroy local instance, calling Destroy callback
        void Destroy();

        // Retrieve object instance by ID
        static std::shared_ptr<Object> GetByID(ObjectID search_id);

        // Local update, calling Update callback
        void Update(float delta);

        // Find a free objectID
        ObjectID GetFreeID();

        // Draw all objects
        static void DrawAll();
};

// Helper function
uint16_t pop_uint16(std::vector<uint8_t>& data);

#endif